#pragma once

#include "core/config.h"

namespace nvidia::reflex {
    void SetOptions(core::config::ReflexMode mode);

    void BeginFrame();
    void EndSimulation();

    void BeginSubmit();
    void EndSubmit();

    void BeginPresent();
    void EndPresent();
}  // namespace nvidia::reflex
