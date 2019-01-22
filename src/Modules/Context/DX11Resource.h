#pragma once

#include <d3d11.h>

#include "Context/Resource.h"

class DX11Resource : public Resource
{
public:
    using Ptr = std::shared_ptr<DX11Resource>;
    ComPtr<ID3D11Resource> resource;
    std::vector<ComPtr<ID3D11Buffer>> tile_pool;

    ComPtr<ID3D11SamplerState> sampler;

    virtual void SetName(const std::string& name) override
    {
        resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.c_str());
    }
};
