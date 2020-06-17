#pragma once
#include <Instance/QueryInterface.h>
#include <Instance/BaseTypes.h>
#include <cstdint>

class Fence : public QueryInterface
{
public:
    virtual ~Fence() = default;
    virtual void Wait(uint64_t value) = 0;
};
