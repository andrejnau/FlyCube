#include "Resource/DXResource.h"

ID3D12Resource* DXResource::GetResource() const
{
    return nullptr;
}

const D3D12_RESOURCE_DESC& DXResource::GetResourceDesc() const
{
    static constexpr D3D12_RESOURCE_DESC kResourceDesc = {};
    return kResourceDesc;
}

const D3D12_SAMPLER_DESC& DXResource::GetSamplerDesc() const
{
    static constexpr D3D12_SAMPLER_DESC kSamplerDesc = {};
    return kSamplerDesc;
}

D3D12_GPU_VIRTUAL_ADDRESS DXResource::GetAccelerationStructureAddress() const
{
    return 0;
}
