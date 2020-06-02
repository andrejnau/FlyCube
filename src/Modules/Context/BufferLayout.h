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
    ViewProvider(const std::shared_ptr<Device>& device, const uint8_t* src_data, BufferLayout& layout);
    ResourceLazyViewDesc GetView(CommandListBox& command_list) override;

private:
    bool SyncData();

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
