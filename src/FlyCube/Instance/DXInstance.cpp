#include "Instance/DXInstance.h"

#include "Adapter/DXAdapter.h"
#include "Utilities/DXUtility.h"
#include "Utilities/SystemUtils.h"

#include <directx/d3d12.h>
#include <dxgi1_6.h>

#include <filesystem>

namespace {

bool EnableAgilitySDKIfExist(uint32_t version, const std::string_view& path)
{
    auto d3d12_core = std::filesystem::u8path(GetExecutableDir()) / path / "D3D12Core.dll";
    if (!std::filesystem::exists(d3d12_core)) {
        return false;
    }

    HMODULE d3d12 = GetModuleHandleA("d3d12.dll");
    assert(d3d12);
    auto D3D12GetInterfacePfn = (PFN_D3D12_GET_INTERFACE)GetProcAddress(d3d12, "D3D12GetInterface");
    if (!D3D12GetInterfacePfn) {
        return false;
    }

    ComPtr<ID3D12SDKConfiguration> sdk_configuration;
    if (FAILED(D3D12GetInterfacePfn(CLSID_D3D12SDKConfiguration, IID_PPV_ARGS(&sdk_configuration)))) {
        return false;
    }
    if (FAILED(sdk_configuration->SetSDKVersion(version, path.data()))) {
        return false;
    }
    return true;
}

} // namespace

#ifdef AGILITY_SDK_REQUIRED
#define EXPORT_AGILITY_SDK extern "C" _declspec(dllexport) extern
#else
#define EXPORT_AGILITY_SDK
#endif

EXPORT_AGILITY_SDK const UINT D3D12SDKVersion = 4;
EXPORT_AGILITY_SDK const char* D3D12SDKPath = ".\\D3D12\\";

#ifndef AGILITY_SDK_REQUIRED
static bool optional_agility_sdk = EnableAgilitySDKIfExist(D3D12SDKVersion, D3D12SDKPath);
#endif

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
    static const bool debug_enabled = IsDebuggerPresent();
    if (debug_enabled) {
        ComPtr<ID3D12Debug> debug_controller;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)))) {
            debug_controller->EnableDebugLayer();
        }
        /*ComPtr<ID3D12Debug1> debug_controller1;
        debug_controller.As(&debug_controller1);
        if (debug_controller1)
            debug_controller1->SetEnableSynchronizedCommandQueueValidation(true);*/
        flags = DXGI_CREATE_FACTORY_DEBUG;
    }

    ASSERT_SUCCEEDED(CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_dxgi_factory)));
}

std::vector<std::shared_ptr<Adapter>> DXInstance::EnumerateAdapters()
{
    std::vector<std::shared_ptr<Adapter>> adapters;

    ComPtr<IDXGIFactory6> dxgi_factory6;
    m_dxgi_factory.As(&dxgi_factory6);

    auto NextAdapted = [&](uint32_t adapter_index, ComPtr<IDXGIAdapter1>& adapter) {
        if (dxgi_factory6) {
            return dxgi_factory6->EnumAdapterByGpuPreference(adapter_index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                                             IID_PPV_ARGS(&adapter));
        } else {
            return m_dxgi_factory->EnumAdapters1(adapter_index, &adapter);
        }
    };

    ComPtr<IDXGIAdapter1> adapter;
    uint32_t gpu_index = 0;
    for (uint32_t adapter_index = 0; DXGI_ERROR_NOT_FOUND != NextAdapted(adapter_index, adapter); ++adapter_index) {
        DXGI_ADAPTER_DESC1 desc = {};
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            continue;
        }

        adapters.emplace_back(std::make_shared<DXAdapter>(*this, adapter));
    }
    return adapters;
}

ComPtr<IDXGIFactory4> DXInstance::GetFactory()
{
    return m_dxgi_factory;
}
