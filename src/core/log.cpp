#include "core/log.h"

#include "core/paths.h"

#include <chrono>
#include <memory>

#include <spdlog/sinks/basic_file_sink.h>

namespace core::log {

    void InitLog() {
        spdlog::set_default_logger(spdlog::basic_logger_mt("JEPlus", core::paths::GetLogFilePath().string(), true));
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%P:%t] %v");
#ifdef NDEBUG
        spdlog::set_level(spdlog::level::info);
#else
        spdlog::set_level(spdlog::level::debug);
#endif
        spdlog::flush_every(std::chrono::seconds(3));
        spdlog::flush_on(spdlog::level::err);
    }

    std::shared_ptr<spdlog::logger> Log() {
        return spdlog::default_logger();
    }

}  // namespace core::log
