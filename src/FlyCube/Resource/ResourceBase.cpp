#include "Resource/ResourceBase.h"

#include <cstring>

ResourceBase::ResourceBase() = default;

ResourceType ResourceBase::GetResourceType() const
{
    return m_resource_type;
}

gli::format ResourceBase::GetFormat() const
{
    return m_format;
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
                                                     uint64_t buffer_row_pitch,
                                                     uint64_t buffer_slice_pitch,
                                                     const void* src_data,
                                                     uint64_t src_row_pitch,
                                                     uint64_t src_slice_pitch,
                                                     uint64_t row_size_in_bytes,
                                                     uint32_t num_rows,
                                                     uint32_t num_slices)
{
    void* dst_data = Map() + buffer_offset;
    for (uint32_t z = 0; z < num_slices; ++z) {
        uint8_t* dest_slice = reinterpret_cast<uint8_t*>(dst_data) + buffer_slice_pitch * z;
        const uint8_t* src_slice = reinterpret_cast<const uint8_t*>(src_data) + src_slice_pitch * z;
        for (uint32_t y = 0; y < num_rows; ++y) {
            memcpy(dest_slice + buffer_row_pitch * y, src_slice + src_row_pitch * y, row_size_in_bytes);
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
    return m_is_back_buffer;
}

void ResourceBase::SetInitialState(ResourceState state)
{
    m_initial_state = state;
}
