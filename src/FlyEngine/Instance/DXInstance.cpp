#include "Instance/DXInstance.h"
#include <Adapter/DXAdapter.h>
#include <Utilities/DXUtility.h>
#include <dxgi1_6.h>

DXInstance::DXInstance()
{
    ASSERT_SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgi_factory)));
}

std::vector<std::unique_ptr<Adapter>> DXInstance::EnumerateAdapters()
{
    std::vector<std::unique_ptr<Adapter>> adapters;

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

        adapters.emplace_back(std::make_unique<DXAdapter>(adapter));
    }
    return adapters;
}
