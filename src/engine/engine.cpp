#include "engine/engine.h"

#include "core/memory.h"

#include <cstddef>
#include <cstdint>

namespace engine {
    // Only the fields this mod uses. The padding sizes must keep each field at its offset in the game
#pragma pack(push, 1)
    struct SwapchainManager {
        void* swapchain;
    };

    struct RenderGlobals {
        std::byte padding0[0x58];
        SwapchainManager* swapchainManager;
    };
#pragma pack(pop)

    static_assert(offsetof(SwapchainManager, swapchain) == 0x0);
    static_assert(offsetof(RenderGlobals, swapchainManager) == 0x58);

    static constexpr uintptr_t RenderGlobalsOffset = 0x23905F8;

    SwapchainState GetSwapchainState() {
        const auto* renderGlobals =
            *reinterpret_cast<RenderGlobals* const*>(core::memory::GetAddress(RenderGlobalsOffset));

        if (renderGlobals == nullptr) {
            return SwapchainState::NotCreated;
        }

        if (!core::memory::IsData(reinterpret_cast<uintptr_t>(renderGlobals), sizeof(RenderGlobals))) {
            return SwapchainState::Unknown;
        }

        const auto* manager = renderGlobals->swapchainManager;

        if (manager == nullptr) {
            return SwapchainState::NotCreated;
        }

        if (!core::memory::IsData(reinterpret_cast<uintptr_t>(manager), sizeof(SwapchainManager))) {
            return SwapchainState::Unknown;
        }

        return manager->swapchain != nullptr ? SwapchainState::Exists : SwapchainState::NotCreated;
    }
}  // namespace engine
