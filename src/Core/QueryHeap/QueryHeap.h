#pragma once
#include "Instance/BaseTypes.h"
#include "Instance/QueryInterface.h"

class QueryHeap : public QueryInterface {
public:
    virtual ~QueryHeap() = default;
    virtual QueryHeapType GetType() const = 0;
};
