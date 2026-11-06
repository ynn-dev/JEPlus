#pragma once

#include <cstddef>
#include <cstdint>

namespace engine::offsets {
    inline constexpr uintptr_t SwapchainCreateOffset = 0x176AC20;
    inline constexpr uintptr_t RenderSubmitOffset = 0x16FB200;
    inline constexpr uintptr_t PresentRoutineOffset = 0x16F9300;
    inline constexpr uintptr_t DlssEvaluateOffset = 0x174AAA0;
    inline constexpr uintptr_t FrameTickOffset = 0xC64770;
    inline constexpr uintptr_t FrameInputOffset = 0x1C3F110;
    inline constexpr uintptr_t FrameBuildOffset = 0x1C3EC60;

    // call to render begin inside the frame
    inline constexpr size_t RenderBeginCall = 0x7C;

    // Inside the Present routine. A hook right after the call would overlap the PresentCall hook
    inline constexpr size_t PresentCall = 0x1A5;   // IDXGISwapChain::Present call
    inline constexpr size_t AfterPresent = 0x1F5;  // first instruction all result paths reach
}  // namespace engine::offsets
