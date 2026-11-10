#include <Windows.h>

#include "core/config.h"
#include "core/log.h"
#include "core/memory.h"
#include "core/paths.h"
#include "core/utils.h"
#include "engine/engine.h"
#include "engine/frame_hooks.h"
#include "engine/offsets.h"
#include "engine/swapchain_hooks.h"

#include <array>
#include <cstdint>
#include <string_view>

using namespace engine::offsets;

struct CodeTargetCheck {
    std::string_view name;
    uintptr_t offset;  // from the image base
    std::array<uint8_t, 8> expectedBytes;
};

static constexpr std::array Targets{
    // 4C 8B DC 53 55 56 57 41 54 41 56 41 57 48 81 EC C0 00 00 00
    CodeTargetCheck{"swapchain create", SwapchainCreateOffset, {0x4C, 0x8B, 0xDC, 0x53, 0x55, 0x56, 0x57, 0x41}},

    // 48 89 5C 24 ?? 48 89 6C 24 ?? 57 48 83 EC 20 48 8B 05 ?? ?? ?? ?? 33 FF
    CodeTargetCheck{"render submit", RenderSubmitOffset, {0x48, 0x89, 0x5C, 0x24, 0x18, 0x48, 0x89, 0x6C}},

    // 44 88 44 24 ?? 89 54 24 ?? 48 89 4C 24 ?? 53
    CodeTargetCheck{"Present routine", PresentRoutineOffset, {0x44, 0x88, 0x44, 0x24, 0x18, 0x89, 0x54, 0x24}},

    // 48 8B C4 44 88 48 ?? 44 89 40
    CodeTargetCheck{"DLSS Evaluate", DlssEvaluateOffset, {0x48, 0x8B, 0xC4, 0x44, 0x88, 0x48, 0x20, 0x44}},

    // 40 53 48 83 EC 20 8B 15 ?? ?? ?? ?? 48 8B D9 8B C2 25 1F 00 00 80 7D ?? FF C8 83 C8 E0 FF C0 85 C0 75 ?? 33 C9 FF
    // 15
    CodeTargetCheck{"frame tick", FrameTickOffset, {0x40, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x8B, 0x15}},

    // 40 53 48 83 EC 30 48 C7 44 24 20 FE FF FF FF 33 DB 38 1D ?? ?? ?? ?? 74 ?? 48 8B 05 ?? ?? ?? ?? 48 85 C0 75 ?? B9
    // E8 01 00 00 E8
    CodeTargetCheck{"frame input", FrameInputOffset, {0x40, 0x53, 0x48, 0x83, 0xEC, 0x30, 0x48, 0xC7}},
};

static bool CheckTargets() {
    bool ok = true;

    for (const auto& target : Targets) {
        if (!core::memory::BytesMatch(core::memory::GetAddress(target.offset), target.expectedBytes)) {
            spdlog::error("Target {} doesn't match", target.name);
            ok = false;
        }
    }

    return ok;
}

static bool RequireSite(std::string_view name, bool passed) {
    if (!passed) {
        spdlog::error("Call site check failed: {}", name);
    }

    return passed;
}

static bool CheckCallSites() {
    using core::memory::BytesMatch;
    using core::memory::GetAddress;
    using core::memory::GetCallAddress;

    static constexpr std::array<uint8_t, 3> PresentCallBytes{0xFF, 0x50, 0x40};
    static constexpr std::array<uint8_t, 7> AfterPresentBytes{0x48, 0x8B, 0x9F, 0xD8, 0x67, 0x00, 0x00};
    static constexpr std::array<uint8_t, 3> CreateSwapChainCallBytes{0x41, 0xFF, 0xD2};

    const auto frameTick = GetAddress(FrameTickOffset);
    const auto frameInput = GetAddress(FrameInputOffset);
    const auto frameBuild = GetAddress(FrameBuildOffset);
    const auto presentRoutine = GetAddress(PresentRoutineOffset);
    const auto swapchainCreate = GetAddress(SwapchainCreateOffset);

    bool ok = true;
    ok &= RequireSite("frame tick +0x62 calls frame input", GetCallAddress(frameTick + 0x62) == frameInput);
    ok &=
        RequireSite("frame input +0x6F calls the pad poll", GetCallAddress(frameInput + 0x6F) == GetAddress(0x1A04200));
    ok &= RequireSite("frame tick +0x75 calls the frame build", GetCallAddress(frameTick + 0x75) == frameBuild);
    ok &= RequireSite("frame build +0x7C calls render begin",
                      GetCallAddress(frameBuild + RenderBeginCall) == GetAddress(0x174C1E0));
    ok &= RequireSite("Present call", BytesMatch(presentRoutine + PresentCall, PresentCallBytes));
    ok &= RequireSite("after Present", BytesMatch(presentRoutine + AfterPresent, AfterPresentBytes));
    ok &= RequireSite("CreateSwapChainForHwnd call",
                      BytesMatch(swapchainCreate + CreateSwapChainCall, CreateSwapChainCallBytes));

    if (ok) {
        spdlog::info("All call sites checked");
    }

    return ok;
}

// nv Streamline can only start when the swapchain is first created, so the hooks have to be enabled before that
static void LogSwapchainTiming() {
    const auto state = engine::GetSwapchainState();

    if (state == engine::SwapchainState::Exists) {
        spdlog::error("The swapchain already exists after installing the hooks, so Streamline could not start");
    } else if (state == engine::SwapchainState::Unknown) {
        spdlog::warn("The swapchain state is unknown");
    } else {
        spdlog::info("The swapchain doesn't exist yet");
    }
}

static DWORD WINAPI MainThread(void*) {
    core::log::InitLog();

    if (!core::utils::IsCorrectExe()) {
        spdlog::warn("Loaded into unexpected process: {} (expected {})", core::paths::GetGameExePath().string(),
                     core::utils::ExecutableName);

        MessageBoxW(nullptr,
                    L"JEPlus is only compatible with the Steam version of Judgment.\n\n"
                    L"The mod will not be loaded into this process.",
                    L"JEPlus", MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);

        return 0;
    }

    core::config::Config config;
    core::config::Load(config);

    spdlog::info("----------");
    spdlog::info("JEPlus loaded");
    spdlog::info("Mod Directory: {}", core::paths::GetAsiPluginDirectory().string());
    spdlog::info("Log File: {}", core::paths::GetLogFilePath().string());
    spdlog::info("Config File: {} ({})", core::paths::GetConfigFilePath().string(),
                 std::filesystem::exists(core::paths::GetConfigFilePath()) ? "found" : "missing");
    spdlog::info("Game Exe Path: {}", core::paths::GetGameExePath().string());
    spdlog::info("Game Module Address: {:p}", (void*)(GetModuleHandleW(nullptr)));
    spdlog::info("Working Directory: {}", std::filesystem::current_path().string());
    spdlog::info("Windows Version: {}", core::utils::GetWindowsVersionString());
    spdlog::info("Log Level: {}", spdlog::level::to_string_view(spdlog::get_level()));
    core::config::Log(config);
    spdlog::info("----------");

    if (!CheckTargets() || !CheckCallSites()) {
        spdlog::error("Game code checks failed");
        return 0;
    }

    // config::Load turns Reflex on whenever Frame Generation is enabled, so Reflex off means neither feature is wanted
    const bool startStreamline = config.nv_reflex_mode != core::config::ReflexMode::Off;

    if (!engine::frame_hooks::Install() || !engine::swapchain_hooks::Install(startStreamline)) {
        return 0;
    }

    LogSwapchainTiming();

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID /*lpReserved*/) {
    if (dwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);

        if (HANDLE thread = CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr)) {
            CloseHandle(thread);
        }
    }

    if (dwReason == DLL_PROCESS_DETACH) {
        spdlog::shutdown();
    }

    return TRUE;
}