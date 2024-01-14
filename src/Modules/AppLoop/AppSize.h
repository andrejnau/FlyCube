#pragma once

#include <cstdint>

struct AppSize {
    constexpr AppSize(uint32_t width, uint32_t height)
        : m_width(width)
        , m_height(height)
    {
    }

    constexpr uint32_t width() const
    {
        return m_width;
    }

    constexpr uint32_t height() const
    {
        return m_height;
    }

private:
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};
