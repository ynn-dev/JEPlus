#include "core/config.h"

#include "core/log.h"
#include "core/paths.h"

#include <glaze/glaze.hpp>
#include <glaze/toml.hpp>

#include <string>

using namespace core::config;

template <>
struct glz::meta<FrameGenerationMode> {
    using enum FrameGenerationMode;
    static constexpr auto value = enumerate("disabled", Disabled, "2x", Static2x, "3x", Static3x, "4x", Static4x, "5x",
                                            Static5x, "6x", Static6x, "dynamic", Dynamic);
};

template <>
struct glz::meta<SuperResolutionUpgrade> {
    using enum SuperResolutionUpgrade;
    static constexpr auto value = enumerate("off", Off, "on", On);
};

template <>
struct glz::meta<ReflexMode> {
    using enum ReflexMode;
    static constexpr auto value = enumerate("off", Off, "on", On, "on_boost", OnBoost);
};

namespace core::config {
    void Load(Config& config) {
        // if anything is wrong, use default config
        Config parsed{};
        std::string buffer{};
        const auto ec = glz::read_file_toml(parsed, core::paths::GetConfigFilePath().string(), buffer);

        if (ec) {
            spdlog::warn("Failed to load config, using defaults: {}", glz::format_error(ec, buffer));
            return;
        }

        if (parsed.dlss_fg_mode != FrameGenerationMode::Disabled && parsed.nv_reflex_mode == ReflexMode::Off) {
            spdlog::warn("Config: DLSS FG needs Reflex, thus using nv_reflex_mode = \"on\"");
            parsed.nv_reflex_mode = ReflexMode::On;
        }

        config = parsed;
    }

    void Log(const Config& config) {
        spdlog::info("Mod Config: Reflex Mode: {} - DLSS SR Upgrade: {} - DLSS FG Mode: {}",
                     glz::get_enum_name(config.nv_reflex_mode), glz::get_enum_name(config.dlss_sr_upgrade),
                     glz::get_enum_name(config.dlss_fg_mode));
    }
}  // namespace core::config
