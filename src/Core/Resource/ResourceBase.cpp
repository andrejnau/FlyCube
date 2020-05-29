#include "Resource/ResourceBase.h"

ResourceType ResourceBase::GetResourceType() const
{
    return resource_type;
}

gli::format ResourceBase::GetFormat() const
{
    return format;
}

MemoryType ResourceBase::GetMemoryType() const
{
    return memory_type;
}

ResourceState ResourceBase::GetResourceState() const
{
    return m_state;
}

void ResourceBase::SetResourceState(ResourceState state)
{
    m_state = state;
}

void ResourceBase::UpdateUploadData(const void* data, uint64_t offset, uint64_t num_bytes)
{
    void* dst_data = Map();
    memcpy(dst_data, data, num_bytes);
    Unmap();
}

void ResourceBase::UpdateSubresource(uint64_t buffer_offset, uint32_t buffer_row_pitch, uint32_t buffer_depth_pitch,
    const void* src_data, uint32_t src_row_pitch, uint32_t src_depth_pitch, uint32_t num_rows, uint32_t num_slices)
{
    void* dst_data = Map();
    for (uint32_t z = 0; z < num_slices; ++z)
    {
        uint8_t* dest_slice = reinterpret_cast<uint8_t*>(dst_data) + buffer_depth_pitch * z;
        const uint8_t* src_slice = reinterpret_cast<const uint8_t*>(src_data) + src_depth_pitch * z;
        for (uint32_t y = 0; y < num_rows; ++y)
        {
            memcpy(dest_slice + buffer_row_pitch * y, src_slice + src_row_pitch * y, src_row_pitch);
        }
    }
    Unmap();
}

void ResourceBase::SetPrivateResource(uint64_t id, const std::shared_ptr<Resource>& resource)
{
    m_private_resources[id] = resource;
}

std::shared_ptr<Resource>& ResourceBase::GetPrivateResource(uint64_t id)
{
    return m_private_resources[id];
}
