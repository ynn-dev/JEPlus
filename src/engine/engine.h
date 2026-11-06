#pragma once

namespace engine {
    enum class SwapchainState { NotCreated, Exists, Unknown };

    SwapchainState GetSwapchainState();
}  // namespace engine