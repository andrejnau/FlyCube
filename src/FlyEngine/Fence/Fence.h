#pragma once
#include <cstdint>

class Fence
{
public:
    virtual ~Fence() = default;
    virtual void Wait() = 0;
    virtual void Reset() = 0;
};
