#include "Resource/ResourceBase.h"

#include "Utilities/NotReached.h"

#include <cstring>

ResourceBase::ResourceBase() = default;

ResourceType ResourceBase::GetResourceType() const
{
    return resource_type_;
}

gli::format ResourceBase::GetFormat() const
{
    return format_;
}

MemoryType ResourceBase::GetMemoryType() const
{
    return memory_type_;
}

uint64_t ResourceBase::GetWidth() const
{
    return 1;
}

uint32_t ResourceBase::GetHeight() const
{
    return 1;
}
uint16_t ResourceBase::GetLayerCount() const
{
    return 1;
}
uint16_t ResourceBase::GetLevelCount() const
{
    return 1;
}
uint32_t ResourceBase::GetSampleCount() const
{
    return 1;
}

uint64_t ResourceBase::GetAccelerationStructureHandle() const
{
    return 0;
}

uint8_t* ResourceBase::Map()
{
    NOTREACHED();
}

void ResourceBase::Unmap()
{
    NOTREACHED();
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
    return initial_state_;
}

bool ResourceBase::IsBackBuffer() const
{
    return is_back_buffer_;
}

void ResourceBase::SetInitialState(ResourceState state)
{
    initial_state_ = state;
}
