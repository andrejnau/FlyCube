#pragma once
#include "Instance/BaseTypes.h"
#include "Instance/QueryInterface.h"

#include <memory>
#include <vector>

class BindingSet : public QueryInterface {
public:
    virtual ~BindingSet() = default;
    virtual void WriteBindings(const std::vector<BindingDesc>& bindings) = 0;
};
