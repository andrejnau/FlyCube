#pragma once
#include <Instance/QueryInterface.h>
#include <Instance/BaseTypes.h>
#include <memory>

class RenderPass : public QueryInterface
{
public:
    virtual ~RenderPass() = default;
    virtual const RenderPassDesc& GetDesc() const = 0;
};
