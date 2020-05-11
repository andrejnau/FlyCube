#pragma once
#include "Resource/Resource.h"
#include <map>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXResource : public Resource
{
public:
    ComPtr<ID3D12Resource> default_res;
    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
    uint32_t bind_flag = 0;
    uint32_t buffer_size = 0;
    uint32_t stride = 0;
    D3D12_RESOURCE_DESC desc = {};

    D3D12_SAMPLER_DESC sampler_desc = {};

    struct
    {
        std::shared_ptr<DXResource> scratch;
        std::shared_ptr<DXResource> result;
        std::shared_ptr<DXResource> instance_desc;
    } as;

    virtual void SetName(const std::string& name) override;
    ComPtr<ID3D12Resource>& GetUploadResource(size_t subresource);

private:
    std::vector<ComPtr<ID3D12Resource>> m_upload_res;
};
