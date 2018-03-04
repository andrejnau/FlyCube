#pragma once

#include <Context/Context.h>
#include <Context/Resource.h>
#include <vector>
#include <dxgiformat.h>

class IAVertexBuffer
{
public:
    template<typename T>
    IAVertexBuffer(Context& context, const std::vector<T>& v)
        : m_context(context)
        , m_stride(sizeof(v.front()))
        , m_offset(0)
        , m_size(v.size() * sizeof(v.front()))
    {
        m_buffer = m_context.CreateBuffer(BindFlag::kVbv, v.size() * sizeof(v.front()), 0);
        if (m_buffer)
            m_context.UpdateSubresource(m_buffer, 0, v.data(), 0, 0);
    }

    void BindToSlot(UINT slot)
    {
        if (m_buffer)
            m_context.IASetVertexBuffer(slot, m_buffer, m_size, m_stride);
    }

private:
    Context & m_context;
    Resource::Ptr m_buffer;
    UINT m_stride;
    UINT m_offset;
    UINT m_size;
};

class IAIndexBuffer
{
public:
    template<typename T>
    IAIndexBuffer(Context& context, const std::vector<T>& v, DXGI_FORMAT format)
        : m_context(context)
        , m_format(format)
        , m_offset(0)
        , m_size(v.size() * sizeof(v.front()))
    {
        m_buffer = m_context.CreateBuffer(BindFlag::kIbv, m_size, 0);
        if (m_buffer)
            m_context.UpdateSubresource(m_buffer, 0, v.data(), 0, 0);
    }

    void Bind()
    {
        if (m_buffer)
            m_context.IASetIndexBuffer(m_buffer, m_size, m_format);
    }

private:
    Context & m_context;
    Resource::Ptr m_buffer;
    UINT m_offset;
    UINT m_size;
    DXGI_FORMAT m_format;
};
