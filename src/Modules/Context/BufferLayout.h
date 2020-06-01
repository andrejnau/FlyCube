#pragma once
#include <Context/Context.h>

struct BufferLayout
{
    size_t dst_buffer_size;
    std::vector<size_t> data_size;
    std::vector<size_t> src_offset;
    std::vector<size_t> dst_offset;
};

class ViewProvider : public DeferredView
{
public:
    ViewProvider(const std::shared_ptr<Device>& device, const uint8_t* src_data, BufferLayout& layout)
        : m_device(device)
        , m_src_data(src_data)
        , m_layout(layout)
        , m_dst_data(layout.dst_buffer_size)
    {
        m_resource = m_device->CreateBuffer(BindFlag::kConstantBuffer, static_cast<uint32_t>(m_layout.dst_buffer_size), MemoryType::kDefault);
    }

    ResourceLazyViewDesc GetView(CommandList& command_list) override
    {
        ResourceLazyViewDesc desc = {};
        if (SyncData() || !m_resource)
        {
            m_resource = m_device->CreateBuffer(BindFlag::kConstantBuffer, static_cast<uint32_t>(m_layout.dst_buffer_size), MemoryType::kUpload);
            m_resource->UpdateUploadData(m_dst_data.data(), 0, m_dst_data.size());
        }
        desc.resource = m_resource;
        return desc;
    }

private:
    bool SyncData()
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

    std::shared_ptr<Device> m_device;
    const uint8_t* m_src_data;
    BufferLayout& m_layout;
    std::vector<uint8_t> m_dst_data;
    std::shared_ptr<Resource> m_resource;
};

template<typename T>
class ConstantBuffer : public T
{
public:
    ConstantBuffer(Context& context, BufferLayout& layout)
    {
        T& data = static_cast<T&>(*this);
        m_view_provider = std::make_shared<ViewProvider>(context.GetDevice(), reinterpret_cast<const uint8_t*>(&data), layout);
    }

    operator std::shared_ptr<DeferredView>& ()
    {
        return m_view_provider;
    }

private:
    std::shared_ptr<DeferredView> m_view_provider;
};
