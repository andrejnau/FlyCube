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
        m_buffer = m_context.CreateBuffer(BindFlag::kVbv, static_cast<uint32_t>(v.size() * sizeof(v.front())), 0);
        if (m_buffer)
            m_context.UpdateSubresource(m_buffer, 0, v.data(), 0, 0);
    }

    void BindToSlot(uint32_t slot)
    {
        if (m_buffer)
            m_context.IASetVertexBuffer(slot, m_buffer, static_cast<uint32_t>(m_size), static_cast<uint32_t>(m_stride));
    }

private:
    Context & m_context;
    Resource::Ptr m_buffer;
    size_t m_stride;
    size_t m_offset;
    size_t m_size;
};

class IAIndexBuffer
{
public:
    template<typename T>
    IAIndexBuffer(Context& context, const std::vector<T>& v, DXGI_FORMAT format)
        : m_context(context)
        , m_format(format)
        , m_offset(0)
        , m_count(v.size())
        , m_size(m_count * sizeof(v.front()))

    {
        m_buffer = m_context.CreateBuffer(BindFlag::kIbv, static_cast<uint32_t>(m_size), 0);
        if (m_buffer)
            m_context.UpdateSubresource(m_buffer, 0, v.data(), 0, 0);
    }

    void Bind()
    {
        if (m_buffer)
            m_context.IASetIndexBuffer(m_buffer, static_cast<uint32_t>(m_size), m_format);
    }

    size_t Count() const
    {
        return m_count;
    }

private:
    Context & m_context;
    Resource::Ptr m_buffer;
    size_t m_offset;
    size_t m_count;
    size_t m_size;
    DXGI_FORMAT m_format;
};
