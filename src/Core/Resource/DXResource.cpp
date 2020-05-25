#include "Resource/DXResource.h"
#include <View/DXView.h>
#include <Device/DXDevice.h>
#include <Utilities/FileUtility.h>
#include <d3dx12.h>

DXResource::DXResource(DXDevice& device)
    : m_device(device)
{
}

ResourceType DXResource::GetResourceType() const
{
    return res_type;
}

gli::format DXResource::GetFormat() const
{
    return m_format;
}

MemoryType DXResource::GetMemoryType() const
{
    return memory_type;
}

uint64_t DXResource::GetWidth() const
{
    return desc.Width;
}

uint32_t DXResource::GetHeight() const
{
    return desc.Height;
}

uint16_t DXResource::GetDepthOrArraySize() const
{
    return desc.DepthOrArraySize;
}

uint16_t DXResource::GetMipLevels() const
{
    return desc.MipLevels;
}

void DXResource::SetName(const std::string& name)
{
    default_res->SetName(utf8_to_wstring(name).c_str());
}

uint8_t* DXResource::Map()
{
    CD3DX12_RANGE range(0, 0);
    uint8_t* dst_data = nullptr;
    ASSERT_SUCCEEDED(default_res->Map(0, &range, reinterpret_cast<void**>(&dst_data)));
    return dst_data;
}

void DXResource::Unmap()
{
    CD3DX12_RANGE range(0, 0);
    default_res->Unmap(0, &range);
}

void DXResource::UpdateUploadData(const void* data, uint64_t offset, uint64_t num_bytes)
{
    CD3DX12_RANGE range(0, 0);
    uint8_t* dst_data = nullptr;
    ASSERT_SUCCEEDED(default_res->Map(0, &range, reinterpret_cast<void**>(&dst_data)));
    memcpy(dst_data + offset, data, num_bytes);
    default_res->Unmap(0, &range);
}

void DXResource::UpdateSubresource(uint64_t buffer_offset, uint32_t buffer_row_pitch, uint32_t buffer_depth_pitch,
                                   const void* src_data, uint32_t src_row_pitch, uint32_t src_depth_pitch, uint32_t num_rows, uint32_t num_slices)
{
    CD3DX12_RANGE range(0, 0);
    uint8_t* dst_data = nullptr;
    ASSERT_SUCCEEDED(default_res->Map(0, &range, reinterpret_cast<void**>(&dst_data)));
    dst_data += buffer_offset;

    for (uint32_t z = 0; z < num_slices; ++z)
    {
        uint8_t* dest_slice = reinterpret_cast<uint8_t*>(dst_data) + buffer_depth_pitch * z;
        const uint8_t* src_slice = reinterpret_cast<const uint8_t*>(src_data) + src_depth_pitch * z;
        for (UINT y = 0; y < num_rows; ++y)
        {
            memcpy(dest_slice + buffer_row_pitch * y, src_slice + src_row_pitch * y, src_row_pitch);
        }
    }

    default_res->Unmap(0, &range);
}
