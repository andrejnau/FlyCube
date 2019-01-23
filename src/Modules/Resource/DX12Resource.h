#pragma once

#include <d3d12.h>
#include <map>

#include "Resource/Resource.h"
#include "Context/BaseTypes.h"
#include "Context/DX12View.h"

class DX12Context;

class DX12Resource : public Resource
{
public:
    using Ptr = std::shared_ptr<DX12Resource>;
    ComPtr<ID3D12Resource> default_res;
    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
    uint32_t bind_flag = 0;
    uint32_t buffer_size = 0;
    uint32_t stride = 0;
    D3D12_RESOURCE_DESC desc = {};

    D3D12_SAMPLER_DESC sampler_desc = {};

    DX12Resource(DX12Context& context);
    ~DX12Resource();

    virtual void SetName(const std::string& name) override;
    ComPtr<ID3D12Resource>& GetUploadResource(size_t subresource);

private:
    DX12Context& m_context;
    std::vector<ComPtr<ID3D12Resource>> m_upload_res;
};
