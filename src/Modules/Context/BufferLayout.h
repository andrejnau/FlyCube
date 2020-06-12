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
    ViewProvider(Device& device, const uint8_t* src_data, BufferLayout& layout);
    std::shared_ptr<ResourceLazyViewDesc> GetView(CommandListBox& command_list) override;
    void OnDestroy(ResourceLazyViewDesc& view_desc) override;

private:
    bool SyncData();

    Device& m_device;
    const uint8_t* m_src_data;
    BufferLayout& m_layout;
    std::vector<uint8_t> m_dst_data;
    std::shared_ptr<ResourceLazyViewDesc> m_last_view;
    std::vector<std::shared_ptr<Resource>> m_free_resources;
};

template<typename T>
class ConstantBuffer : public T
{
public:
    ConstantBuffer(Device& device, BufferLayout& layout)
    {
        T& data = static_cast<T&>(*this);
        m_view_provider = std::make_shared<ViewProvider>(device, reinterpret_cast<const uint8_t*>(&data), layout);
    }

    operator std::shared_ptr<DeferredView>& ()
    {
        return m_view_provider;
    }

private:
    std::shared_ptr<DeferredView> m_view_provider;
};
