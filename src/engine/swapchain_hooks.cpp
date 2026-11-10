#include "engine/swapchain_hooks.h"

#include "core/log.h"
#include "core/memory.h"
#include "engine/offsets.h"
#include "nvidia/streamline.h"

#include <dxgi1_2.h>
#include <safetyhook.hpp>

#include <cstdint>

namespace engine::swapchain_hooks {
    using namespace offsets;

    static safetyhook::InlineHook orgSwapchainCreate;
    static safetyhook::MidHook createSwapChainCallHook;
    static core::config::ReflexMode reflexMode = core::config::ReflexMode::Off;
    static bool streamlineSwapchain = false;

    static void CreateSwapChainForHwnd_AddWaitableFlag(safetyhook::Context& ctx) {
        if (!streamlineSwapchain) {
            return;
        }

        auto* desc = reinterpret_cast<DXGI_SWAP_CHAIN_DESC1*>(ctx.r9);
        const auto before = desc->Flags;
        desc->Flags |=
            DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;  // fix frame pacing (stuck at 30 fps otherwise)
        spdlog::debug("CreateSwapChainForHwnd: flags 0x{:X} -> 0x{:X}, {}x{} format {}", before, desc->Flags,
                      desc->Width, desc->Height, static_cast<uint32_t>(desc->Format));
    }

    static int64_t __fastcall SwapchainCreate_Detour(void* manager, void* parameters) {
        spdlog::debug("Swapchain create (manager {})", manager);

        // if reflex is inactive, FG is automatically inactive and thus there is no need for streamline
        if (reflexMode != core::config::ReflexMode::Off) {
            nvidia::streamline::Init(reflexMode);
            streamlineSwapchain = nvidia::streamline::UseUpgradedFactory();
        }

        const auto result = orgSwapchainCreate.call<int64_t>(manager, parameters);
        spdlog::info("Swapchain created: {} (result 0x{:08X})", streamlineSwapchain ? "Streamline" : "native",
                     static_cast<uint32_t>(result));
        return result;
    }

    bool Install(core::config::ReflexMode mode) {
        reflexMode = mode;
        orgSwapchainCreate =
            safetyhook::create_inline(core::memory::GetAddress(SwapchainCreateOffset), SwapchainCreate_Detour);

        if (!orgSwapchainCreate) {
            spdlog::error("Swapchain create hook failed");
            return false;
        }

        // 41 FF D2 (call r10, IDXGIFactory2::CreateSwapChainForHwnd)
        createSwapChainCallHook =
            safetyhook::create_mid(core::memory::GetAddress(SwapchainCreateOffset) + CreateSwapChainCall,
                                   CreateSwapChainForHwnd_AddWaitableFlag);

        if (!createSwapChainCallHook) {
            spdlog::error("CreateSwapChainForHwnd hook failed");
            orgSwapchainCreate = {};
            return false;
        }

        spdlog::info("Swapchain hooks installed");
        return true;
    }
}  // namespace engine::swapchain_hooks
