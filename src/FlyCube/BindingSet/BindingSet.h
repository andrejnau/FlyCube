#pragma once
#include "Instance/BaseTypes.h"
#include "Instance/QueryInterface.h"

class BindingSet : public QueryInterface {
public:
    virtual ~BindingSet() = default;
    virtual void WriteBindings(const WriteBindingsDesc& desc) = 0;
};
