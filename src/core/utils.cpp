#include "core/utils.h"

#include "core/log.h"
#include "core/paths.h"

#include <Windows.h>
#include <winternl.h>

#include <format>

#pragma comment(lib, "ntdll.lib")

namespace core::utils {
    // ntdll.dll
    extern "C" NTSTATUS NTAPI RtlGetVersion(PRTL_OSVERSIONINFOW lpVersionInformation);

    std::string GetWindowsVersionString() {
        RTL_OSVERSIONINFOW info{};
        info.dwOSVersionInfoSize = sizeof(info);

        if (!NT_SUCCESS(RtlGetVersion(&info))) {
            return "unknown";
        }

        return std::format("{}.{}.{}", info.dwMajorVersion, info.dwMinorVersion, info.dwBuildNumber);
    }

    // TODO: move to a dedicated consts header?
    static constexpr DWORD ExpectedTimeDateStamp = 0x64140255;
    static constexpr DWORD ExpectedSizeOfImage = 0x19257000;
    static constexpr DWORD ExpectedEntryPoint = 0x190DF020;

    bool IsCorrectExe() {
        const std::wstring filename = core::paths::GetGameExePath().filename().wstring();

        // case-insensitive comparison
        if (_wcsicmp(filename.c_str(), std::wstring(ExecutableName.begin(), ExecutableName.end()).c_str()) != 0) {
            spdlog::error("Unexpected executable: {} (expected {})", core::paths::GetGameExePath().string(),
                          ExecutableName);
            return false;
        }

        const uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
        const IMAGE_DOS_HEADER* dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        const IMAGE_NT_HEADERS64* ntHeader = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dosHeader->e_lfanew);

        const auto& fileHeader = ntHeader->FileHeader;
        const auto& optionalHeader = ntHeader->OptionalHeader;

        if (fileHeader.TimeDateStamp != ExpectedTimeDateStamp || optionalHeader.SizeOfImage != ExpectedSizeOfImage ||
            optionalHeader.AddressOfEntryPoint != ExpectedEntryPoint) {
            spdlog::error("Unsupported build: TimeDateStamp 0x{:08X}, SizeOfImage 0x{:X}, entry 0x{:X}",
                          fileHeader.TimeDateStamp, optionalHeader.SizeOfImage, optionalHeader.AddressOfEntryPoint);
            return false;
        }

        return true;
    }
}  // namespace core::utils
