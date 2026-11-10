#include "engine/frame_hooks.h"

#include "core/log.h"
#include "core/memory.h"
#include "engine/offsets.h"
#include "nvidia/reflex.h"

#include <safetyhook.hpp>

#include <cstdint>

namespace engine::frame_hooks {
    using namespace offsets;

    static safetyhook::InlineHook orgFrameInput;
    static safetyhook::InlineHook orgRenderSubmit;
    static safetyhook::InlineHook orgDlssEvaluate;
    static safetyhook::MidHook renderBeginHook;
    static safetyhook::MidHook presentStartHook;
    static safetyhook::MidHook presentEndHook;

    static int64_t __fastcall FrameInput_Detour() {
        nvidia::reflex::BeginFrame();
        return orgFrameInput.call<int64_t>();
    }

    static int64_t __fastcall RenderSubmit_Detour(void* manager) {
        nvidia::reflex::BeginSubmit();
        const auto result = orgRenderSubmit.call<int64_t>(manager);
        nvidia::reflex::EndSubmit();
        return result;
    }

    static void* __fastcall DlssEvaluate_Detour(void* context, void* parameters, unsigned frameIndex, char reset) {
        return orgDlssEvaluate.call<void*>(context, parameters, frameIndex, reset);
    }

    // No branch lands inside this call, so a mid-hook can relocate it
    static void RenderBegin_Detour(safetyhook::Context&) {
        nvidia::reflex::EndSimulation();
    }

    static void PresentStart_Detour(safetyhook::Context&) {
        nvidia::reflex::BeginPresent();
    }

    static void PresentEnd_Detour(safetyhook::Context&) {
        nvidia::reflex::EndPresent();
    }

    bool Install() {
        using core::memory::GetAddress;

        orgFrameInput = safetyhook::create_inline(GetAddress(FrameInputOffset), FrameInput_Detour);
        orgRenderSubmit = safetyhook::create_inline(GetAddress(RenderSubmitOffset), RenderSubmit_Detour);
        orgDlssEvaluate = safetyhook::create_inline(GetAddress(DlssEvaluateOffset), DlssEvaluate_Detour);
        renderBeginHook = safetyhook::create_mid(GetAddress(FrameBuildOffset) + RenderBeginCall, RenderBegin_Detour);
        presentStartHook = safetyhook::create_mid(GetAddress(PresentRoutineOffset) + PresentCall, PresentStart_Detour);
        presentEndHook = safetyhook::create_mid(GetAddress(PresentRoutineOffset) + AfterPresent, PresentEnd_Detour);

        const bool ok = orgFrameInput && orgRenderSubmit && orgDlssEvaluate && renderBeginHook && presentStartHook &&
                        presentEndHook;

        if (!ok) {
            spdlog::error(
                "One or more frame hooks failed: input={} submit={} evaluate={} renderBegin={} presentStart={} "
                "presentEnd={}",
                static_cast<bool>(orgFrameInput), static_cast<bool>(orgRenderSubmit),
                static_cast<bool>(orgDlssEvaluate), static_cast<bool>(renderBeginHook),
                static_cast<bool>(presentStartHook), static_cast<bool>(presentEndHook));

            // All or nothing, so the ones that did go in come out again
            presentEndHook = {};
            presentStartHook = {};
            renderBeginHook = {};
            orgDlssEvaluate = {};
            orgRenderSubmit = {};
            orgFrameInput = {};
            return false;
        }

        spdlog::info("Frame hooks installed");
        return true;
    }
}  // namespace engine::frame_hooks
