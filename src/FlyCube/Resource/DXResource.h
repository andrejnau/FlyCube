#pragma once
#include "Resource/ResourceBase.h"

#if !defined(_WIN32)
#include <wsl/winadapter.h>
#endif

#include <directx/d3d12.h>

class DXResource : public ResourceBase {
public:
    virtual ID3D12Resource* GetResource() const;
    virtual const D3D12_RESOURCE_DESC& GetResourceDesc() const;
    virtual const D3D12_SAMPLER_DESC& GetSamplerDesc() const;
    virtual D3D12_GPU_VIRTUAL_ADDRESS GetAccelerationStructureAddress() const;
};
