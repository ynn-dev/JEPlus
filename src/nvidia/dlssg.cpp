#include "nvidia/dlssg.h"

#include "core/log.h"
#include "nvidia/streamline.h"

#include <sl_helpers.h>

namespace nvidia::dlssg {
    void Reset() {
        const auto* functions = streamline::GetDlssgFunctions();

        if (functions == nullptr) {
            return;
        }

        const sl::ViewportHandle viewport{0u};

        // placeholder settings
        sl::DLSSGOptions options{};
        options.mode = sl::DLSSGMode::eOff;
        options.numFramesToGenerate = 1;
        options.flags = sl::DLSSGFlags::eRetainResourcesWhenOff;

        const auto optionsResult = functions->setOptions(viewport, options);

        sl::DLSSGState state{};
        const auto stateResult = functions->getState(viewport, state, nullptr);

        spdlog::info("DLSS-G: set off: {}", sl::getResultAsStr(optionsResult));

        if (stateResult != sl::Result::eOk) {
            spdlog::warn("DLSS-G: slDLSSGGetState failed: {}", sl::getResultAsStr(stateResult));
            return;
        }

        const auto maxMultiplier = state.numFramesToGenerateMax + 1;
        const auto isDynamicMultiplierSupported = state.bIsDynamicMFGSupported == sl::Boolean::eTrue;

        spdlog::info("DLSS-G: status 0x{:X}, multiplier up to {}x, dynamic {}", static_cast<uint32_t>(state.status),
                     maxMultiplier, isDynamicMultiplierSupported);
    }
}  // namespace nvidia::dlssg
