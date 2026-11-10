#include "nvidia/reflex.h"

#include "core/log.h"
#include "nvidia/streamline.h"

#include <sl_helpers.h>

#include <atomic>

namespace nvidia::reflex {
    using core::config::ReflexMode;

    static std::atomic<sl::FrameToken*> frameToken{nullptr};  // nullptr when no frame is open

    static void SetMarker(sl::PCLMarker marker) {
        const auto* functions = streamline::GetReflexFunctions();

        if (functions == nullptr) {
            return;
        }

        auto* token = frameToken.load(std::memory_order_acquire);

        if (token == nullptr) {
            return;
        }

        functions->setMarker(marker, *token);
    }

    static void OpenFrame(bool sleep) {
        const auto* functions = streamline::GetReflexFunctions();

        if (functions == nullptr) {
            return;
        }

        sl::FrameToken* token = nullptr;

        functions->getNewFrameToken(token, nullptr);
        frameToken.store(token, std::memory_order_release);

        if (token == nullptr) {
            return;
        }

        if (sleep) {
            functions->sleep(*token);
        }

        SetMarker(sl::PCLMarker::eSimulationStart);
    }

    void SetOptions(ReflexMode mode) {
        const auto* functions = streamline::GetReflexFunctions();

        if (functions == nullptr) {
            return;
        }

        sl::ReflexOptions options{};

        switch (mode) {
        case ReflexMode::Off: options.mode = sl::ReflexMode::eOff; break;
        case ReflexMode::On: options.mode = sl::ReflexMode::eLowLatency; break;
        case ReflexMode::OnBoost: options.mode = sl::ReflexMode::eLowLatencyWithBoost; break;
        }

        const auto result = functions->setOptions(options);
        spdlog::info("Reflex: mode {} -> {}", sl::getReflexModeAsStr(options.mode), sl::getResultAsStr(result));
    }

    void BeginFrame() {
        OpenFrame(true);
    }

    void EndSimulation() {
        SetMarker(sl::PCLMarker::eSimulationEnd);
    }

    void BeginSubmit() {
        if (frameToken.load(std::memory_order_acquire) == nullptr) {
            OpenFrame(false);
            SetMarker(sl::PCLMarker::eSimulationEnd);  // solves some edge cases
        }

        SetMarker(sl::PCLMarker::eRenderSubmitStart);
    }

    void EndSubmit() {
        SetMarker(sl::PCLMarker::eRenderSubmitEnd);
    }

    void BeginPresent() {
        SetMarker(sl::PCLMarker::ePresentStart);
    }

    void EndPresent() {
        SetMarker(sl::PCLMarker::ePresentEnd);
        frameToken.store(nullptr, std::memory_order_release);
    }
}  // namespace nvidia::reflex
