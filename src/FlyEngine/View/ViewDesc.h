#pragma once
#include <cstdint>

struct ViewDesc
{
    ViewDesc() = default;

    ViewDesc(size_t level, size_t count = -1)
        : level(level)
        , count(count)
    {
    }

    bool operator<(const ViewDesc& oth) const
    {
        return level < oth.level;
    }

    size_t level = 0;
    size_t count = -1;
};
