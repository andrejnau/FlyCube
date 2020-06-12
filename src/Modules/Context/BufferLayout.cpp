#include "Context/BufferLayout.h"

static constexpr bool use_copy = false;

ViewProvider::ViewProvider(Device& device, const uint8_t* src_data, BufferLayout& layout)
    : m_device(device)
    , m_src_data(src_data)
    , m_layout(layout)
    , m_dst_data(layout.dst_buffer_size)
{
    if (use_copy)
        m_resource = m_device.CreateBuffer(BindFlag::kConstantBuffer, static_cast<uint32_t>(m_layout.dst_buffer_size), MemoryType::kDefault);
}

ResourceLazyViewDesc ViewProvider::GetView(CommandListBox& command_list)
{
    if (use_copy)
    {
        if (SyncData())
            command_list.UpdateSubresource(m_resource, 0, m_dst_data.data());
    }
    else if (SyncData() || !m_resource)
    {
        auto upload = m_device.CreateBuffer(BindFlag::kConstantBuffer, static_cast<uint32_t>(m_layout.dst_buffer_size), MemoryType::kUpload);
        upload->UpdateUploadData(m_dst_data.data(), 0, m_dst_data.size());
        m_resource = upload;
    }
    return { m_resource };
}

bool ViewProvider::SyncData()
{
    bool dirty = false;
    for (size_t i = 0; i < m_layout.data_size.size(); ++i)
    {
        const uint8_t* ptr_src = m_src_data + m_layout.src_offset[i];
        uint8_t* ptr_dst = m_dst_data.data() + m_layout.dst_offset[i];
        if (std::memcmp(ptr_dst, ptr_src, m_layout.data_size[i]) != 0)
        {
            std::memcpy(ptr_dst, ptr_src, m_layout.data_size[i]);
            dirty = true;
        }
    }
    return dirty;
}
