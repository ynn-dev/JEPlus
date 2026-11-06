#pragma once

#include <spdlog/spdlog.h>

namespace core::log {

    void InitLog();
    std::shared_ptr<spdlog::logger> Log();

}  // namespace core::log
