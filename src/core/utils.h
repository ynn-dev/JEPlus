#pragma once

#include <Windows.h>

#include <string>
#include <string_view>

namespace core::utils {
    inline constexpr std::string_view ExecutableName = "Judgment.exe";

    bool IsCorrectExe();
    std::string GetWindowsVersionString();
}  // namespace core::utils
