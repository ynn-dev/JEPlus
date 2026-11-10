#pragma once

struct ID3D12Device;
struct IDXGIFactory2;

namespace engine {
    enum class SwapchainState { NotCreated, Exists, Unknown };

    SwapchainState GetSwapchainState();

    ID3D12Device* GetDevice();
    IDXGIFactory2* GetFactory();
    bool SetFactory(IDXGIFactory2* factory);
}  // namespace engine
