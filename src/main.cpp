#include <Windows.h>

#include "core/config.h"
#include "core/log.h"
#include "core/paths.h"
#include "core/utils.h"

DWORD WINAPI MainThread(void*) {
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