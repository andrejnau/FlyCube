#pragma once

#include <d3d12.h>
#include <map>

#include "Context/Resource.h"
#include "Context/BaseTypes.h"

class DescriptorHeapRange;
class DX12Context;

class DX12Resource : public Resource
{
public:
    using Ptr = std::shared_ptr<DX12Resource>;
    ComPtr<ID3D12Resource> default_res;
    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
    std::map<BindKey, DescriptorHeapRange> descriptors;
    uint32_t stride = 0;
    D3D12_RESOURCE_DESC desc = {};

    DX12Resource(DX12Context& context);
    ~DX12Resource();

    virtual void SetName(const std::string& name) override;
    ComPtr<ID3D12Resource>& GetUploadResource(size_t subresource);

private:
    DX12Context& m_context;
    std::vector<ComPtr<ID3D12Resource>> m_upload_res;
};
