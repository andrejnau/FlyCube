#pragma once
#include "Resource/Resource.h"

class ResourceBase : public Resource
{
public:
    virtual uint8_t* Map() = 0;
    virtual void Unmap() = 0;

    ResourceType GetResourceType() const override final;
    gli::format GetFormat() const override final;
    MemoryType GetMemoryType() const override final;
    ResourceState GetResourceState(uint32_t mip_level, uint32_t array_layer) const override final;
    void SetResourceState(ResourceState state) override final;
    void SetResourceState(uint32_t mip_level, uint32_t array_layer, ResourceState state) override final;

    void UpdateUploadData(const void* data, uint64_t offset, uint64_t num_bytes) override final;
    void UpdateSubresource(uint64_t buffer_offset, uint32_t buffer_row_pitch, uint32_t buffer_depth_pitch,
        const void* src_data, uint32_t src_row_pitch, uint32_t src_depth_pitch, uint32_t num_rows, uint32_t num_slices) override final;
    void SetPrivateResource(uint64_t id, const std::shared_ptr<Resource>& resource) override final;
    std::shared_ptr<Resource>& GetPrivateResource(uint64_t id)  override final;

    gli::format format = gli::FORMAT_UNDEFINED;
    MemoryType memory_type = MemoryType::kDefault;
    ResourceType resource_type = ResourceType::kUnknown;

private:
    std::map<uint64_t, std::shared_ptr<Resource>> m_private_resources;
    std::map<std::tuple<uint32_t, uint32_t>, ResourceState> m_states;
    ResourceState m_state = ResourceState::kUndefined;
};
