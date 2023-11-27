#pragma once
#include "Instance/BaseTypes.h"
#include "Instance/QueryInterface.h"

#include <cstdint>

class Fence : public QueryInterface {
public:
    virtual ~Fence() = default;
    virtual uint64_t GetCompletedValue() = 0;
    virtual void Wait(uint64_t value) = 0;
    virtual void Signal(uint64_t value) = 0;
};
