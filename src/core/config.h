#pragma once

namespace core::config {
    enum class ReflexMode { Off = 0, On = 1, OnBoost = 2 };
    enum class SuperResolutionUpgrade { Off, On };
    enum class FrameGenerationMode { Disabled, Static2x, Static3x, Static4x, Static5x, Static6x, Dynamic };

    struct Config {
        ReflexMode nv_reflex_mode{ReflexMode::On};
        SuperResolutionUpgrade dlss_sr_upgrade{SuperResolutionUpgrade::On};
        FrameGenerationMode dlss_fg_mode{FrameGenerationMode::Disabled};
    };

    void Load(Config& config);
    void Log(const Config& config);
}  // namespace core::config
