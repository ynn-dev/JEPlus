#pragma once

#include "core/config.h"

#include <sl.h>
#include <sl_dlss_g.h>
#include <sl_pcl.h>
#include <sl_reflex.h>

namespace nvidia::streamline {
    struct DlssgFunctions {
        PFun_slDLSSGSetOptions* setOptions;
        PFun_slDLSSGGetState* getState;
    };

    struct ReflexFunctions {
        PFun_slGetNewFrameToken* getNewFrameToken;
        PFun_slReflexSleep* sleep;
        PFun_slReflexSetOptions* setOptions;
        PFun_slPCLSetMarker* setMarker;
    };

    void Init(const core::config::Config& config);
    bool UseUpgradedFactory();

    // Null until Streamline runs with Reflex loaded, with or without DLSS Frame Generation
    const ReflexFunctions* GetReflexFunctions();

    // Null unless DLSS FG is wanted, supported and loaded
    const DlssgFunctions* GetDlssgFunctions();
}  // namespace nvidia::streamline
