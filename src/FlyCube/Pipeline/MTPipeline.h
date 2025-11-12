#pragma once
#include "Instance/BaseTypes.h"
#include "Pipeline/Pipeline.h"
#include "Shader/Shader.h"

class MTPipeline : public Pipeline {
public:
    virtual std::shared_ptr<Shader> GetShader(ShaderType type) const = 0;
};
