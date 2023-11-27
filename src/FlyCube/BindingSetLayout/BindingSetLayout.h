#pragma once
#include "Instance/BaseTypes.h"
#include "Instance/QueryInterface.h"

#include <memory>
#include <vector>

class BindingSetLayout : public QueryInterface {
public:
    virtual ~BindingSetLayout() = default;
};
