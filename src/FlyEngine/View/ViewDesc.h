#pragma once
#include <cstdint>
#include <Instance/BaseTypes.h>

struct ViewDesc
{
    size_t level = 0;
    size_t count = -1;
    ResourceDimension dimension;
    uint32_t stride = 0;
    ResourceType res_type;
};
