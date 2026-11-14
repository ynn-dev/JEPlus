#include "paths.h"

#include "Windows.h"

#include <optional>
#include <string>

namespace core::paths {
    // handle of the plugin DLL
    static HMODULE GetPluginHandle() {
        HMODULE moduleHandle = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<const wchar_t*>(&GetPluginHandle), &moduleHandle);
        return moduleHandle;
    }

    // empty on failure
    static std::filesystem::path GetModulePath(HMODULE moduleHandle) {
        constexpr DWORD MaxPathLength = 0x8000;
        std::wstring buffer(MaxPathLength, L'\0');

        const DWORD length = GetModuleFileNameW(moduleHandle, buffer.data(), static_cast<DWORD>(buffer.size()));

        if (length == 0 || length >= buffer.size()) {
            return {};
        }

        buffer.resize(length);

        return std::filesystem::path(buffer);
    }

    static std::optional<std::filesystem::path> GetModuleDirectory(HMODULE moduleHandle) {
        const auto modulePath = GetModulePath(moduleHandle);
        if (modulePath.empty()) {
            return std::nullopt;
        }

        const std::filesystem::path directory = modulePath.parent_path();
        if (directory.empty()) {
            return std::nullopt;
        }

        return directory;
    }

    // <Steam library>\runtime\media\Judgment.exe
    std::filesystem::path GetGameExePath() {
        static const auto exePath = GetModulePath(nullptr);
        return exePath;
    }

    // <Steam library>\runtime\media
    std::filesystem::path GetGameDirectory() {
        static const auto directory = GetModuleDirectory(nullptr).value_or(std::filesystem::path{});
        return directory;
    }

    // <Steam library>\runtime\media\mods\JEPlus probably
    std::filesystem::path GetAsiPluginDirectory() {
        static const auto directory = GetModuleDirectory(GetPluginHandle()).value_or(std::filesystem::path{});
        return directory;
    }

    std::filesystem::path GetGameFilePath(std::wstring_view name) {
        return GetGameDirectory() / std::filesystem::path(name);
    }

    std::filesystem::path GetConfigFilePath() {
        return GetAsiPluginDirectory() / L"JEPlus.toml";
    }

    std::filesystem::path GetLogDirectory() {
        return GetAsiPluginDirectory() / L"logs";
    }

    std::filesystem::path GetLogFilePath() {
        return GetLogDirectory() / L"JEPlus.log";
    }
}  // namespace core::paths