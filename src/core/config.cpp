#include "core/config.h"

#include "core/log.h"
#include "core/paths.h"

#include <glaze/glaze.hpp>
#include <glaze/toml.hpp>

using namespace core::config;

template <>
struct glz::meta<FrameGenerationMode> {
    using enum FrameGenerationMode;
    static constexpr auto value = enumerate("disabled", DISABLED, "2x", STATIC_2X, "3x", STATIC_3X, "4x", STATIC_4X,
                                            "5x", STATIC_5X, "6x", STATIC_6X, "dynamic", DYNAMIC);
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

        if (parsed.dlss_fg_mode != FrameGenerationMode::DISABLED && parsed.nv_reflex_mode == ReflexMode::Off) {
            spdlog::warn("Config: DLSS FG needs Reflex, thus using nv_reflex_mode = \"on\"");
            parsed.nv_reflex_mode = ReflexMode::On;
        }

        config = parsed;
    }

    void Log(const Config& config) {
        spdlog::info("Mod Config: Reflex Mode: {} - DLSS FG Mode: {}", glz::get_enum_name(config.nv_reflex_mode),
                     glz::get_enum_name(config.dlss_fg_mode));
    }
}  // namespace core::config
