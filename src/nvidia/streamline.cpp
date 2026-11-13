#include "nvidia/streamline.h"

#include "core/log.h"
#include "core/paths.h"
#include "engine/engine.h"
#include "nvidia/reflex.h"

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_2.h>

#include <sl.h>
#include <sl_helpers.h>
#include <sl_security.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace nvidia::streamline {
    static PFun_slIsFeatureLoaded* slIsFeatureLoaded = nullptr;
    static PFun_slSetFeatureLoaded* slSetFeatureLoaded = nullptr;

    // TODO: there's gotta be a more elegant way to handle this
    // using an atomic primitive here to avoid thread headaches in the future
    static ReflexFunctions reflexFunctions{};
    static std::atomic<const ReflexFunctions*> reflex{nullptr};
    static DlssgFunctions dlssgFunctions{};
    static std::atomic<const DlssgFunctions*> dlssg{nullptr};

    // keep both the game and the streamline swapchain to allow switching between them
    static IDXGIFactory2* nativeFactory = nullptr;
    static IDXGIFactory2* upgradedFactory = nullptr;

    static bool Succeeded(const char* call, sl::Result result) {
        if (result != sl::Result::eOk) {
            spdlog::error("Streamline: {} failed: {}", call, sl::getResultAsStr(result));
            return false;
        }

        return true;
    }

    template <typename Function>
    static Function* GetExport(HMODULE module, const char* name) {
        return reinterpret_cast<Function*>(GetProcAddress(module, name));
    }

    // slInit doesn't always leave a requested feature loaded, so load it here if needed
    static bool EnsureLoaded(sl::Feature feature, const char* name) {
        bool loaded = false;

        slIsFeatureLoaded(feature, loaded);

        if (loaded) {
            return true;
        }

        return Succeeded(name, slSetFeatureLoaded(feature, true));
    }

    // TODO: refactor ResolveReflex and ResolveFrameGeneration because theres gotta be a better way
    static bool ResolveReflex(HMODULE module) {
        const auto slGetFeatureFunction = GetExport<PFun_slGetFeatureFunction>(module, "slGetFeatureFunction");
        const auto slGetNewFrameToken = GetExport<PFun_slGetNewFrameToken>(module, "slGetNewFrameToken");

        if (slGetFeatureFunction == nullptr || slGetNewFrameToken == nullptr) {
            spdlog::error("Streamline: sl.interposer.dll lacks slGetFeatureFunction or slGetNewFrameToken");
            return false;
        }

        void* sleep = nullptr;
        void* setOptions = nullptr;
        void* setMarker = nullptr;
        slGetFeatureFunction(sl::kFeatureReflex, "slReflexSleep", sleep);
        slGetFeatureFunction(sl::kFeatureReflex, "slReflexSetOptions", setOptions);
        slGetFeatureFunction(sl::kFeaturePCL, "slPCLSetMarker", setMarker);

        if (sleep == nullptr || setOptions == nullptr || setMarker == nullptr) {
            spdlog::error(
                "Streamline: missing Reflex functions: slReflexSleep={} slReflexSetOptions={} "
                "slPCLSetMarker={}",
                sleep, setOptions, setMarker);
            return false;
        }

        const ReflexFunctions functions{
            .getNewFrameToken = slGetNewFrameToken,
            .sleep = reinterpret_cast<PFun_slReflexSleep*>(sleep),
            .setOptions = reinterpret_cast<PFun_slReflexSetOptions*>(setOptions),
            .setMarker = reinterpret_cast<PFun_slPCLSetMarker*>(setMarker),
        };

        reflexFunctions = functions;
        reflex.store(&reflexFunctions, std::memory_order_release);
        spdlog::debug("Streamline: Reflex functions resolved: slReflexSleep={} slReflexSetOptions={} slPCLSetMarker={}",
                      sleep, setOptions, setMarker);
        return true;
    }

    static bool ResolveFrameGeneration(HMODULE module) {
        const auto slGetFeatureFunction = GetExport<PFun_slGetFeatureFunction>(module, "slGetFeatureFunction");

        if (slGetFeatureFunction == nullptr) {
            spdlog::error("Streamline: sl.interposer.dll lacks slGetFeatureFunction");
            return false;
        }

        void* setOptions = nullptr;
        void* getState = nullptr;
        slGetFeatureFunction(sl::kFeatureDLSS_G, "slDLSSGSetOptions", setOptions);
        slGetFeatureFunction(sl::kFeatureDLSS_G, "slDLSSGGetState", getState);

        if (setOptions == nullptr || getState == nullptr) {
            spdlog::error("Streamline: missing DLSS-G functions: slDLSSGSetOptions={} slDLSSGGetState={}", setOptions,
                          getState);
            return false;
        }

        dlssgFunctions = DlssgFunctions{
            .setOptions = reinterpret_cast<PFun_slDLSSGSetOptions*>(setOptions),
            .getState = reinterpret_cast<PFun_slDLSSGGetState*>(getState),
        };
        dlssg.store(&dlssgFunctions, std::memory_order_release);
        spdlog::debug("Streamline: DLSS-G functions resolved: slDLSSGSetOptions={} slDLSSGGetState={}", setOptions,
                      getState);
        return true;
    }

    static void Start(const core::config::Config& config) {
        auto* device = engine::GetDevice();
        auto* factory = engine::GetFactory();

        if (device == nullptr || factory == nullptr) {
            spdlog::error("Streamline: the game's D3D12 device or DXGI factory doesn't exist yet");
            return;
        }

        const auto modDirectory = core::paths::GetAsiPluginDirectory();
        const auto pluginDirectory = modDirectory / L"bin";
        const auto interposerPath = pluginDirectory / L"sl.interposer.dll";

        if (!std::filesystem::is_regular_file(interposerPath)) {
            spdlog::error("Streamline: {} is missing", interposerPath.string());
            return;
        }

        // NVIDIA recommends it
        if (!sl::security::verifyEmbeddedSignature(interposerPath.c_str())) {
            spdlog::error("Streamline: {} isn't signed by NVIDIA, so it won't be loaded", interposerPath.string());
            return;
        }

        const HMODULE module = LoadLibraryW(interposerPath.c_str());

        if (module == nullptr) {
            spdlog::error("Streamline: couldn't load {} (error {})", interposerPath.string(), GetLastError());
            return;
        }

        const auto slInit = GetExport<PFun_slInit>(module, "slInit");
        const auto slSetD3DDevice = GetExport<PFun_slSetD3DDevice>(module, "slSetD3DDevice");
        const auto slIsFeatureSupported = GetExport<PFun_slIsFeatureSupported>(module, "slIsFeatureSupported");
        const auto slUpgradeInterface = GetExport<PFun_slUpgradeInterface>(module, "slUpgradeInterface");
        slIsFeatureLoaded = GetExport<PFun_slIsFeatureLoaded>(module, "slIsFeatureLoaded");
        slSetFeatureLoaded = GetExport<PFun_slSetFeatureLoaded>(module, "slSetFeatureLoaded");

        if (slInit == nullptr || slSetD3DDevice == nullptr || slIsFeatureSupported == nullptr ||
            slUpgradeInterface == nullptr || slIsFeatureLoaded == nullptr || slSetFeatureLoaded == nullptr) {
            spdlog::error("Streamline: sl.interposer.dll lacks functions that SDK {}.{}.{} needs", SL_VERSION_MAJOR,
                          SL_VERSION_MINOR, SL_VERSION_PATCH);
            return;
        }

        const std::wstring pluginPath = pluginDirectory.wstring();
        const std::wstring gamePath = core::paths::GetGameDirectory().wstring();
        const std::wstring logPath = modDirectory.wstring();

        // if DLSS SR upgrade is enabled, add the updated DLSS SR DLL folder to the list of paths, otherwise the game
        // will simply load the one shipped with it
        const auto upgradeDirectory = pluginDirectory / L"dlss_sr";
        const std::wstring upgradePath = upgradeDirectory.wstring();
        const bool upgradeSr = config.dlss_sr_upgrade == core::config::SuperResolutionUpgrade::On;

        if (upgradeSr && !std::filesystem::is_regular_file(upgradeDirectory / L"nvngx_dlss.dll")) {
            spdlog::warn("Streamline: {} has no nvngx_dlss.dll, so DLSS SR isn't upgraded", upgradeDirectory.string());
        }

        std::vector<const wchar_t*> pluginPaths{pluginPath.c_str()};

        if (upgradeSr) {
            pluginPaths.push_back(upgradePath.c_str());
        }

        pluginPaths.push_back(gamePath.c_str());

        spdlog::info("Streamline: DLSS SR upgrade: {}", upgradeSr ? "on" : "off");

        const bool wantFrameGeneration = config.dlss_fg_mode != core::config::FrameGenerationMode::Disabled;
        std::vector<sl::Feature> Features{sl::kFeatureReflex};

        if (wantFrameGeneration) {
            Features.push_back(sl::kFeatureDLSS_G);
        }

        sl::Preferences preferences{};
#ifdef NDEBUG
        preferences.logLevel = sl::LogLevel::eDefault;
#else
        preferences.logLevel = sl::LogLevel::eVerbose;
#endif
        preferences.pathsToPlugins = pluginPaths.data();
        preferences.numPathsToPlugins = static_cast<uint32_t>(pluginPaths.size());
        preferences.pathToLogsAndData = logPath.c_str();

        // enable the factory proxy to allow to easily pointer swap the factory
        // TODO: check other flags
        preferences.flags |= sl::PreferenceFlags::eUseDXGIFactoryProxy;
        preferences.featuresToLoad = Features.data();
        preferences.numFeaturesToLoad = static_cast<uint32_t>(Features.size());
        preferences.applicationId = 0x06084F03;  // TODO: check if it's right
        preferences.renderAPI = sl::RenderAPI::eD3D12;

        if (!Succeeded("slInit", slInit(preferences, sl::kSDKVersion)) ||
            !Succeeded("slSetD3DDevice", slSetD3DDevice(device))) {
            return;
        }

        LUID luid = device->GetAdapterLuid();
        sl::AdapterInfo adapter{};
        adapter.deviceLUID = reinterpret_cast<uint8_t*>(&luid);
        adapter.deviceLUIDSizeInBytes = sizeof(luid);

        const auto reflexSupport = slIsFeatureSupported(sl::kFeatureReflex, adapter);
        spdlog::info("Streamline: Reflex support: {}", sl::getResultAsStr(reflexSupport));

        if (reflexSupport != sl::Result::eOk) {
            spdlog::warn("Streamline: Reflex isn't supported on this GPU");
            return;
        }

        // early return if Reflex is not available, as it's required for FG
        if (!EnsureLoaded(sl::kFeatureReflex, "loading Reflex") || !ResolveReflex(module)) {
            return;
        }

        // TODO: works for now, add live switching in the future?
        reflex::SetOptions(config.nv_reflex_mode);

        // TODO: check if integrating MFG unlocker is possible to allow RTX 20 and 30 series cards to run frame gen
        bool frameGeneration = false;

        if (wantFrameGeneration) {
            const auto frameGenerationSupport = slIsFeatureSupported(sl::kFeatureDLSS_G, adapter);
            spdlog::info("Streamline: DLSS Frame Generation support: {}", sl::getResultAsStr(frameGenerationSupport));

            frameGeneration = frameGenerationSupport == sl::Result::eOk &&
                              EnsureLoaded(sl::kFeatureDLSS_G, "loading DLSS Frame Generation") &&
                              ResolveFrameGeneration(module);
        }

        // slUpgradeInterface overwrites the pointer passed to it, so we pass a copy instead of the original
        IDXGIFactory2* upgraded = factory;

        if (!Succeeded("slUpgradeInterface on the DXGI factory",
                       slUpgradeInterface(reinterpret_cast<void**>(&upgraded)))) {
            dlssg.store(nullptr, std::memory_order_release);
            return;
        }

        nativeFactory = factory;
        upgradedFactory = upgraded;
        spdlog::debug("Streamline: factory {} upgraded to {}", static_cast<void*>(nativeFactory),
                      static_cast<void*>(upgradedFactory));
        spdlog::info("Streamline ready: {}", frameGeneration ? "Reflex and DLSS Frame Generation" : "Reflex only");
    }

    bool UseUpgradedFactory() {
        if (upgradedFactory == nullptr) {
            return false;
        }

        if (engine::GetFactory() == upgradedFactory) {
            return true;
        }

        if (engine::GetFactory() != nativeFactory) {
            return false;
        }

        upgradedFactory->AddRef();

        if (!engine::SetFactory(upgradedFactory)) {
            upgradedFactory->Release();
            return false;
        }

        spdlog::info("Streamline: the engine now creates its swapchain through the upgraded factory");
        return true;
    }

    const ReflexFunctions* GetReflexFunctions() {
        return reflex.load(std::memory_order_acquire);
    }

    const DlssgFunctions* GetDlssgFunctions() {
        return dlssg.load(std::memory_order_acquire);
    }

    void Init(const core::config::Config& config) {
        static bool attempted = false;

        if (!std::exchange(attempted, true)) {
            Start(config);
        }
    }
}  // namespace nvidia::streamline
