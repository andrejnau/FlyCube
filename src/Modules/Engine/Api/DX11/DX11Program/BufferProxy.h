#pragma once

#include <cstdint>
#include <cassert>

class BufferProxy
{
public:
    BufferProxy(char* data, size_t size)
        : m_data(data)
        , m_size(size)
    {
    }

    template<typename T>
    void operator=(const T& value)
    {
        assert(sizeof(T) == m_size || m_size == -1);
        *reinterpret_cast<T*>(m_data) = value;
    }

private:
    char* m_data;
    size_t m_size;
};
