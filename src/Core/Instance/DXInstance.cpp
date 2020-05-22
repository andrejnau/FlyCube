#include "Instance/DXInstance.h"
#include <Adapter/DXAdapter.h>
#include <Utilities/DXUtility.h>
#include <dxgi1_6.h>
#include <d3d12.h>

DXInstance::DXInstance()
{
#if 0
    static const GUID D3D12ExperimentalShaderModelsID = { /* 76f5573e-f13a-40f5-b297-81ce9e18933f */
        0x76f5573e,
        0xf13a,
        0x40f5,
    { 0xb2, 0x97, 0x81, 0xce, 0x9e, 0x18, 0x93, 0x3f }
    };
    ASSERT_SUCCEEDED(D3D12EnableExperimentalFeatures(1, &D3D12ExperimentalShaderModelsID, nullptr, nullptr));
#endif

    uint32_t flags = 0;
#if defined(_DEBUG) || 1
    ComPtr<ID3D12Debug> debug_controller;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller))))
    {
        debug_controller->EnableDebugLayer();
    }
    flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    ASSERT_SUCCEEDED(CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_dxgi_factory)));
}

std::vector<std::shared_ptr<Adapter>> DXInstance::EnumerateAdapters()
{
    std::vector<std::shared_ptr<Adapter>> adapters;

    ComPtr<IDXGIFactory6> dxgi_factory6;
    m_dxgi_factory.As(&dxgi_factory6);

    auto NextAdapted = [&](uint32_t adapter_index, ComPtr<IDXGIAdapter1>& adapter)
    {
        if (dxgi_factory6)
            return dxgi_factory6->EnumAdapterByGpuPreference(adapter_index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter));
        else
            return m_dxgi_factory->EnumAdapters1(adapter_index, &adapter);
    };

    ComPtr<IDXGIAdapter1> adapter;
    uint32_t gpu_index = 0;
    for (uint32_t adapter_index = 0; DXGI_ERROR_NOT_FOUND != NextAdapted(adapter_index, adapter); ++adapter_index)
    {
        DXGI_ADAPTER_DESC1 desc = {};
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        adapters.emplace_back(std::make_shared<DXAdapter>(*this, adapter));
    }
    return adapters;
}

ComPtr<IDXGIFactory4> DXInstance::GetFactory()
{
    return m_dxgi_factory;
}
