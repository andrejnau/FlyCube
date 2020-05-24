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
    ResourceType GetResourceType() const override;
    gli::format GetFormat() const override;
    MemoryType GetMemoryType() const override;
    uint64_t GetWidth() const override;
    uint32_t GetHeight() const override;
    uint16_t GetDepthOrArraySize() const override;
    uint16_t GetMipLevels() const override;
    void SetName(const std::string& name) override;
    void UpdateUploadData(const void* data, uint64_t offset, uint64_t num_bytes) override;
    void UpdateSubresource(uint32_t texture_subresource, uint64_t buffer_offset, uint32_t buffer_row_pitch, uint32_t buffer_depth_pitch,
                           const void* src_data, uint32_t src_row_pitch, uint32_t src_depth_pitch, uint32_t num_rows, uint32_t num_slices) override;

//private:
    DXDevice& m_device;
    MemoryType memory_type = MemoryType::kDefault;
    ResourceType res_type = ResourceType::kUnknown;
    gli::format m_format = {};
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
};
