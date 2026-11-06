#include "core/memory.h"

#include <Windows.h>

#include <cstring>

namespace core::memory {
    static uintptr_t GetBaseAddress() {
        return reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    }

    uintptr_t GetAddress(uintptr_t offset) {
        return GetBaseAddress() + offset;
    }

    bool BytesMatch(uintptr_t address, std::span<const uint8_t> bytes) {
        return std::memcmp(reinterpret_cast<const void*>(address), bytes.data(), bytes.size()) == 0;
    }

    uintptr_t GetCallAddress(uintptr_t address) {
        if (*reinterpret_cast<const uint8_t*>(address) != 0xE8) {
            return 0;
        }

        // memcpy because the displacement can be unaligned
        int32_t displacement = 0;
        std::memcpy(&displacement, reinterpret_cast<const void*>(address + 1), sizeof(displacement));
        return address + 5 + displacement;
    }

    bool IsData(uintptr_t address, size_t size) {
        MEMORY_BASIC_INFORMATION info{};

        if (VirtualQuery(reinterpret_cast<const void*>(address), &info, sizeof(info)) == 0) {
            return false;
        }

        const bool isWritableData = info.State == MEM_COMMIT &&
                                    (info.Protect & (PAGE_READWRITE | PAGE_WRITECOPY)) != 0 &&
                                    (info.Protect & PAGE_GUARD) == 0;
        const auto regionEnd = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;

        return isWritableData && address + size <= regionEnd;
    }
}  // namespace core::memory