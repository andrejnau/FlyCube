#pragma once
#include "Pipeline/Pipeline.h"
#include <Instance/BaseTypes.h>

class MTPipeline : public Pipeline
{
public:
    virtual std::shared_ptr<Program> GetProgram() const = 0;
};
