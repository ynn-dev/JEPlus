#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace core::memory {
    uintptr_t GetAddress(uintptr_t offset);  // returns base address + offset

    bool BytesMatch(uintptr_t address,
                    std::span<const uint8_t> bytes);  // used to check if functions in the game image match

    // Get the target address of a call instruction
    uintptr_t GetCallAddress(uintptr_t address);

    // check that it's memory that is readable, writable and not executable
    bool IsData(uintptr_t address, size_t size);
}  // namespace core::memory