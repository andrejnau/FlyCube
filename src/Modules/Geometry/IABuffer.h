#pragma once

#include <Context/Context.h>
#include <Resource/Resource.h>
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
        , m_count(v.size())
    {
        m_buffer = m_context.CreateBuffer(BindFlag::kVbv, static_cast<uint32_t>(v.size() * sizeof(v.front())), static_cast<uint32_t>(m_stride));
        if (m_buffer)
            m_context.UpdateSubresource(m_buffer, 0, v.data(), 0, 0);
    }

    void BindToSlot(uint32_t slot)
    {
        if (m_buffer)
            m_context.IASetVertexBuffer(slot, m_buffer);
    }

    Resource::Ptr GetBuffer() const
    {
        return m_buffer;
    }

    size_t Count() const
    {
        return m_count;
    }

private:
    Context& m_context;
    Resource::Ptr m_buffer;
    size_t m_stride;
    size_t m_offset;
    size_t m_size;
    size_t m_count;
};

class IAIndexBuffer
{
public:
    template<typename T>
    IAIndexBuffer(Context& context, const std::vector<T>& v, gli::format format)
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
            m_context.IASetIndexBuffer(m_buffer, m_format);
    }

    Resource::Ptr GetBuffer() const
    {
        return m_buffer;
    }

    size_t Count() const
    {
        return m_count;
    }

    gli::format Format() const
    {
        return m_format;
    }

private:
    Context& m_context;
    Resource::Ptr m_buffer;
    size_t m_offset;
    size_t m_count;
    size_t m_size;
    gli::format m_format;
};
