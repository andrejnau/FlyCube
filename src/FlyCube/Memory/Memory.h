#pragma once
#include "Instance/BaseTypes.h"
#include "Instance/QueryInterface.h"

class Memory : public QueryInterface {
public:
    virtual ~Memory() = default;
    virtual MemoryType GetMemoryType() const = 0;
};
