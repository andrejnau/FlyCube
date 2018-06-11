#pragma once

#include <d3d11.h>

#include "Context/Resource.h"

class DX11Resource : public Resource
{
public:
    using Ptr = std::shared_ptr<DX11Resource>;
    ComPtr<ID3D11Resource> resource;
    std::vector<ComPtr<ID3D11Buffer>> tile_pool;

    std::map<BindKey, ComPtr<ID3D11ShaderResourceView>> srv;
    std::map<BindKey, ComPtr<ID3D11UnorderedAccessView>> uav;
    std::map<BindKey, ComPtr<ID3D11RenderTargetView>> rtv;
    std::map<BindKey, ComPtr<ID3D11DepthStencilView>> dsv;

    virtual void SetName(const std::string& name) override
    {
        resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.c_str());
    }
};
