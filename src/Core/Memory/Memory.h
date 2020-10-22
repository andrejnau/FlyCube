#pragma once
#include <Instance/QueryInterface.h>
#include <Instance/BaseTypes.h>

class Memory : public QueryInterface
{
public:
    virtual ~Memory() = default;
    virtual MemoryType GetMemoryType() const = 0;
};
