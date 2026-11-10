#pragma once

#include <filesystem>
#include <string_view>

namespace core::paths {
    std::filesystem::path GetGameExePath();

    // directory containing the game executable
    std::filesystem::path GetGameDirectory();

    // get a file from the game path (ex. a DLL)
    std::filesystem::path GetGameFilePath(std::wstring_view name);

    // directory containing the plugin DLL
    std::filesystem::path GetAsiPluginDirectory();

    std::filesystem::path GetConfigFilePath();
    std::filesystem::path GetLogFilePath();
}  // namespace core::paths
