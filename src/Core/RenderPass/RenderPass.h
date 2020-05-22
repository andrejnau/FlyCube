#pragma once
#include <Instance/QueryInterface.h>
#include <Instance/BaseTypes.h>
#include <memory>

class RenderPass : public QueryInterface
{
public:
    virtual ~RenderPass() = default;
};
