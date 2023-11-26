#pragma once
#include "Instance/BaseTypes.h"
#include "Pipeline/Pipeline.h"

class MTPipeline : public Pipeline {
public:
    virtual std::shared_ptr<Program> GetProgram() const = 0;
};
