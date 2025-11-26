#pragma once
#include "Instance/BaseTypes.h"
#include "Instance/QueryInterface.h"

#include <memory>
#include <vector>

class BindingSet : public QueryInterface {
public:
    virtual ~BindingSet() = default;
    virtual void WriteBindings(const std::vector<BindingDesc>& bindings) = 0;
    virtual void WriteBindingsAndConstants(const std::vector<BindingDesc>& bindings,
                                           const std::vector<BindingConstantsData>& constants) = 0;
};
