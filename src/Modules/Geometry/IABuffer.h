#pragma once

#include <Context/Context.h>
#include <Resource/Resource.h>
#include <vector>

class IAVertexBuffer
{
public:
    template<typename T>
    IAVertexBuffer(Context& context, const std::vector<T>& v)
        : m_context(context)
        , m_size(v.size() * sizeof(v.front()))
        , m_count(v.size())
    {
        m_buffer = m_context.CreateBuffer(BindFlag::kVertexBuffer | BindFlag::kShaderResource, static_cast<uint32_t>(m_size));
        if (m_buffer)
            m_context.UpdateSubresource(m_buffer, 0, v.data(), 0, 0);
    }

    void BindToSlot(uint32_t slot)
    {
        if (m_dynamic_buffer)
            m_context.IASetVertexBuffer(slot, m_dynamic_buffer);
        else if (m_buffer)
            m_context.IASetVertexBuffer(slot, m_buffer);
    }

    std::shared_ptr<Resource> GetBuffer() const
    {
        return m_buffer;
    }

    std::shared_ptr<Resource> GetDynamicBuffer()
    {
        if (!m_dynamic_buffer)
            m_dynamic_buffer = m_context.CreateBuffer(BindFlag::kVertexBuffer | BindFlag::kUnorderedAccess, static_cast<uint32_t>(m_size));
        return m_dynamic_buffer;
    }

    size_t Count() const
    {
        return m_count;
    }

    bool IsDynamic() const
    {
        return !!m_dynamic_buffer;
    }

private:
    Context& m_context;
    std::shared_ptr<Resource> m_buffer;
    std::shared_ptr<Resource> m_dynamic_buffer;
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
        , m_count(v.size())
        , m_size(m_count * sizeof(v.front()))
    {
        m_buffer = m_context.CreateBuffer(BindFlag::kIndexBuffer | BindFlag::kShaderResource, static_cast<uint32_t>(m_size));
        if (m_buffer)
            m_context.UpdateSubresource(m_buffer, 0, v.data(), 0, 0);
    }

    void Bind()
    {
        if (m_buffer)
            m_context.IASetIndexBuffer(m_buffer, m_format);
    }

    std::shared_ptr<Resource> GetBuffer() const
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
    std::shared_ptr<Resource> m_buffer;
    size_t m_count;
    size_t m_size;
    gli::format m_format;
};
