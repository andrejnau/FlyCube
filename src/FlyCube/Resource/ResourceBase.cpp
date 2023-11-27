#include "Resource/ResourceBase.h"

#include <cstring>

ResourceBase::ResourceBase()
    : m_resource_state_tracker(*this)
{
}

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
    return m_memory_type;
}

void ResourceBase::UpdateUploadBuffer(uint64_t buffer_offset, const void* data, uint64_t num_bytes)
{
    void* dst_data = Map() + buffer_offset;
    memcpy(dst_data, data, num_bytes);
    Unmap();
}

void ResourceBase::UpdateUploadBufferWithTextureData(uint64_t buffer_offset,
                                                     uint32_t buffer_row_pitch,
                                                     uint32_t buffer_depth_pitch,
                                                     const void* src_data,
                                                     uint32_t src_row_pitch,
                                                     uint32_t src_depth_pitch,
                                                     uint32_t num_rows,
                                                     uint32_t num_slices)
{
    void* dst_data = Map() + buffer_offset;
    for (uint32_t z = 0; z < num_slices; ++z) {
        uint8_t* dest_slice = reinterpret_cast<uint8_t*>(dst_data) + buffer_depth_pitch * z;
        const uint8_t* src_slice = reinterpret_cast<const uint8_t*>(src_data) + src_depth_pitch * z;
        for (uint32_t y = 0; y < num_rows; ++y) {
            memcpy(dest_slice + buffer_row_pitch * y, src_slice + src_row_pitch * y, src_row_pitch);
        }
    }
    Unmap();
}

ResourceState ResourceBase::GetInitialState() const
{
    return m_initial_state;
}

bool ResourceBase::IsBackBuffer() const
{
    return is_back_buffer;
}

void ResourceBase::SetInitialState(ResourceState state)
{
    m_initial_state = state;
    m_resource_state_tracker.SetResourceState(m_initial_state);
}

ResourceStateTracker& ResourceBase::GetGlobalResourceStateTracker()
{
    return m_resource_state_tracker;
}

const ResourceStateTracker& ResourceBase::GetGlobalResourceStateTracker() const
{
    return m_resource_state_tracker;
}
