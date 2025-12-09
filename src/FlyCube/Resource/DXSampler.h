#pragma once
#include "Resource/DXResource.h"
#include "Utilities/PassKey.h"

#if !defined(_WIN32)
#include <wsl/winadapter.h>
#endif

#include <directx/d3d12.h>

class DXDevice;

class DXSampler : public DXResource {
public:
    DXSampler(PassKey<DXSampler> pass_key, DXDevice& device);

    static std::shared_ptr<DXSampler> CreateSampler(DXDevice& device, const SamplerDesc& desc);

    // Resource:
    void SetName(const std::string& name) override;

    // DXResource:
    const D3D12_SAMPLER_DESC& GetSamplerDesc() const override;

private:
    DXDevice& device_;

    D3D12_SAMPLER_DESC sampler_desc_ = {};
};
