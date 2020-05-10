#pragma once
#include <cstdint>

class Fence
{
public:
    virtual ~Fence() = default;
    virtual void WaitAndReset() = 0;
};
