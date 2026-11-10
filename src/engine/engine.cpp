#include "engine/engine.h"

#include "core/memory.h"

#include <cstddef>
#include <cstdint>

namespace engine {
#pragma pack(push, 1)
    struct SwapchainManager {
        void* swapchain;
    };

    struct RenderGlobals {
        std::byte padding0[0x8];
        ID3D12Device* device;
        std::byte padding1[0x40];
        IDXGIFactory2* factory;
        SwapchainManager* swapchainManager;
    };
#pragma pack(pop)

    static_assert(offsetof(SwapchainManager, swapchain) == 0x0);
    static_assert(offsetof(RenderGlobals, device) == 0x08);
    static_assert(offsetof(RenderGlobals, factory) == 0x50);
    static_assert(offsetof(RenderGlobals, swapchainManager) == 0x58);

    static constexpr uintptr_t RenderGlobalsOffset = 0x23905F8;

    static RenderGlobals* GetRenderGlobals() {
        auto* renderGlobals = *reinterpret_cast<RenderGlobals**>(core::memory::GetAddress(RenderGlobalsOffset));

        if (renderGlobals == nullptr ||
            !core::memory::IsData(reinterpret_cast<uintptr_t>(renderGlobals), sizeof(RenderGlobals))) {
            return nullptr;
        }

        return renderGlobals;
    }

    ID3D12Device* GetDevice() {
        const auto* renderGlobals = GetRenderGlobals();

        if (renderGlobals == nullptr) {
            return nullptr;
        }

        return renderGlobals->device;
    }

    IDXGIFactory2* GetFactory() {
        const auto* renderGlobals = GetRenderGlobals();

        if (renderGlobals == nullptr) {
            return nullptr;
        }

        return renderGlobals->factory;
    }

    // replace the game factory with our own; will affect next swapchain
    bool SetFactory(IDXGIFactory2* factory) {
        auto* renderGlobals = GetRenderGlobals();

        if (renderGlobals == nullptr) {
            return false;
        }

        renderGlobals->factory = factory;
        return true;
    }

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
