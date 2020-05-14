#pragma once
#include <BindingSet/BindingSet.h>
#include <Instance/BaseTypes.h>
#include <memory>

class Program
{
public:
    virtual ~Program() = default;
    virtual std::shared_ptr<BindingSet> CreateBindingSet(const std::vector<BindingDesc>& bindings) = 0;
};
