#include "Context/DXGIUtility.h"

ComPtr<IDXGIAdapter1> GetHardwareAdapter(const ComPtr<IDXGIFactory4>& dxgi_factory)
{
    ComPtr<IDXGIAdapter1> adapter;
    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != dxgi_factory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 desc = {};
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        return adapter;
    }
    return nullptr;
}

ComPtr<IDXGISwapChain3> CreateSwapChain(const ComPtr<IUnknown>& device, const ComPtr<IDXGIFactory4>& dxgi_factory, HWND window, UINT width, UINT height, UINT FrameCount)
{
    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
    swap_chain_desc.Width = width;
    swap_chain_desc.Height = height;
    swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.BufferCount = FrameCount;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    ComPtr<IDXGISwapChain3> swap_chain;
    ComPtr<IDXGISwapChain1> tmp_swap_chain;
    ASSERT_SUCCEEDED(dxgi_factory->CreateSwapChainForHwnd(device.Get(), window, &swap_chain_desc, nullptr, nullptr, &tmp_swap_chain));
    tmp_swap_chain.As(&swap_chain);
    return swap_chain;
}
