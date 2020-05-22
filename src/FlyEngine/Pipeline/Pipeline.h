#pragma once
#include <Instance/QueryInterface.h>
#include <Instance/BaseTypes.h>
#include <memory>

class Pipeline : public QueryInterface
{
public:
    virtual ~Pipeline() = default;
    virtual PipelineType GetPipelineType() const = 0;
};
