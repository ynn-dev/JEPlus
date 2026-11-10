#pragma once

#include "core/config.h"

#include <sl.h>
#include <sl_pcl.h>
#include <sl_reflex.h>

namespace nvidia::streamline {
    struct ReflexFunctions {
        PFun_slGetNewFrameToken* getNewFrameToken;
        PFun_slReflexSleep* sleep;
        PFun_slReflexSetOptions* setOptions;
        PFun_slPCLSetMarker* setMarker;
    };

    void Init(core::config::ReflexMode reflexMode);
    bool UseUpgradedFactory();

    // Null until Streamline runs with Reflex loaded, with or without DLSS Frame Generation
    const ReflexFunctions* GetReflexFunctions();
}  // namespace nvidia::streamline
