#pragma once
#include "Instance/BaseTypes.h"
#include "Instance/QueryInterface.h"

#include <memory>

class RenderPass : public QueryInterface {
public:
    virtual ~RenderPass() = default;
    virtual const RenderPassDesc& GetDesc() const = 0;
};
