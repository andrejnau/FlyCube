#pragma once

#include <cstdint>

struct AppSize {
    constexpr AppSize() = default;

    constexpr AppSize(uint32_t width, uint32_t height)
        : width_(width)
        , height_(height)
    {
    }

    constexpr uint32_t width() const
    {
        return width_;
    }

    constexpr uint32_t height() const
    {
        return height_;
    }

private:
    uint32_t width_ = 0;
    uint32_t height_ = 0;
};
