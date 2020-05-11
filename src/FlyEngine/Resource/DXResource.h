#pragma once
#include "Resource/Resource.h"
#include <map>
#include <vector>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXDevice;

class DXResource : public Resource
{
public:
    DXResource(DXDevice& device);
    void SetName(const std::string& name) override;
    std::shared_ptr<View> CreateView(const ViewDesc& view_desc) override;

//private:
    DXDevice& m_device;
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

    ComPtr<ID3D12Resource>& GetUploadResource(size_t subresource);

    std::vector<ComPtr<ID3D12Resource>> m_upload_res;
};
