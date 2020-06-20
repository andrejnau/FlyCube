#pragma once
#include <Instance/QueryInterface.h>
#include <Instance/BaseTypes.h>
#include <cstdint>

class Fence : public QueryInterface
{
public:
    virtual ~Fence() = default;
    virtual uint64_t GetCompletedValue() = 0;
    virtual void Wait(uint64_t value) = 0;
};
