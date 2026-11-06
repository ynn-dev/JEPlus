#pragma once

#include <string>

namespace core::config {
    enum class ReflexMode { Off = 0, On = 1, OnBoost = 2 };

    enum class FrameGenerationMode { DISABLED, STATIC_2X, STATIC_3X, STATIC_4X, STATIC_5X, STATIC_6X, DYNAMIC };

    struct Config {
        ReflexMode nv_reflex_mode{ReflexMode::On};
        FrameGenerationMode dlss_fg_mode{FrameGenerationMode::DISABLED};
    };

    void Load(Config& config);
    void Log(const Config& config);
}  // namespace core::config
