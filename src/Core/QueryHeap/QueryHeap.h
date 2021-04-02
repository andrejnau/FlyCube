#pragma once
#include <Instance/QueryInterface.h>
#include <Instance/BaseTypes.h>

class QueryHeap : public QueryInterface
{
public:
    virtual ~QueryHeap() = default;
    virtual QueryHeapType GetType() const = 0;
};
