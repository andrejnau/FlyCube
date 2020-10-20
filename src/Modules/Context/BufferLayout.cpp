#include "Context/BufferLayout.h"

ViewProvider::ViewProvider(Device& device, const uint8_t* src_data, BufferLayout& layout)
    : m_device(device)
    , m_src_data(src_data)
    , m_layout(layout)
    , m_dst_data(layout.dst_buffer_size)
{
}

std::shared_ptr<ResourceLazyViewDesc> ViewProvider::GetView(RenderCommandList& command_list)
{
    if (SyncData() || !m_last_view)
    {
        std::shared_ptr<Resource> resource;
        if (!m_free_resources.empty())
        {
            resource = m_free_resources.back();
            m_free_resources.pop_back();
        }
        else
        {
            resource = m_device.CreateBuffer(BindFlag::kConstantBuffer, static_cast<uint32_t>(m_layout.dst_buffer_size), MemoryType::kUpload);
        }
        resource->UpdateUploadData(m_dst_data.data(), 0, m_dst_data.size());
        m_last_view = std::make_shared<ResourceLazyViewDesc>(*this, resource);
    }
    return m_last_view;
}

void ViewProvider::OnDestroy(ResourceLazyViewDesc& view_desc)
{
    m_free_resources.emplace_back(view_desc.resource);
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
