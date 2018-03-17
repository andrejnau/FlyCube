#pragma once

#include <d3d12.h>

#include "Context/Resource.h"

class DX12Resource : public Resource
{
public:
    using Ptr = std::shared_ptr<DX12Resource>;
    ComPtr<ID3D12Resource> default_res;
    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
    size_t stride = 0;

    ComPtr<ID3D12Resource>& GetUploadResource(size_t subresource)
    {
        if (subresource >= upload_res.size())
            upload_res.resize(subresource + 1);
        return upload_res[subresource];
    }

    virtual void SetName(const std::string& name) override
    {
        default_res->SetName(utf8_to_wstring(name).c_str());
    }

private:
    std::vector<ComPtr<ID3D12Resource>> upload_res;
};
