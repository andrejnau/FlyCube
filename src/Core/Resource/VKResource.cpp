#include "Resource/VKResource.h"
#include <View/VKView.h>
#include <Device/VKDevice.h>

VKResource::VKResource(VKDevice& device)
    : m_device(device)
{
}

ResourceType VKResource::GetResourceType() const
{
    return res_type;
}

gli::format VKResource::GetFormat() const
{
    return m_format;
}

MemoryType VKResource::GetMemoryType() const
{
    return memory_type;
}

uint64_t VKResource::GetWidth() const
{
    if (res_type == ResourceType::kImage)
        return image.size.width;
    return buffer.size;
}

uint32_t VKResource::GetHeight() const
{
    return image.size.height;
}

uint16_t VKResource::GetDepthOrArraySize() const
{
    return image.array_layers;
}

uint16_t VKResource::GetMipLevels() const
{
    return image.level_count;
}

void VKResource::SetName(const std::string& name)
{
}

uint8_t* VKResource::Map()
{
    uint8_t* dst_data = nullptr;
    m_device.GetDevice().mapMemory(buffer.memory.get(), 0, VK_WHOLE_SIZE, {}, reinterpret_cast<void**>(&dst_data));
    return dst_data;
}

void VKResource::Unmap()
{
    m_device.GetDevice().unmapMemory(buffer.memory.get());
}

void VKResource::UpdateUploadData(const void* data, uint64_t offset, uint64_t num_bytes)
{
    void* dst_data = nullptr;
    m_device.GetDevice().mapMemory(buffer.memory.get(), offset, num_bytes, {}, &dst_data);
    memcpy(dst_data, data, num_bytes);
    m_device.GetDevice().unmapMemory(buffer.memory.get());
}

void VKResource::UpdateSubresource(uint64_t buffer_offset, uint32_t buffer_row_pitch, uint32_t buffer_depth_pitch,
                                   const void* src_data, uint32_t src_row_pitch, uint32_t src_depth_pitch, uint32_t num_rows, uint32_t num_slices)
{
    void* dst_data = nullptr;
    m_device.GetDevice().mapMemory(buffer.memory.get(), buffer_offset, VK_WHOLE_SIZE, {}, &dst_data);

    for (uint32_t z = 0; z < num_slices; ++z)
    {
        uint8_t* dest_slice = reinterpret_cast<uint8_t*>(dst_data) + buffer_depth_pitch * z;
        const uint8_t* src_slice = reinterpret_cast<const uint8_t*>(src_data) + src_depth_pitch * z;
        for (UINT y = 0; y < num_rows; ++y)
        {
            memcpy(dest_slice + buffer_row_pitch * y, src_slice + src_row_pitch * y, src_row_pitch);
        }
    }

    m_device.GetDevice().unmapMemory(buffer.memory.get());
}
