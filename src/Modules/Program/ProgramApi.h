#pragma once

#include "Program/ShaderType.h"
#include "Program/ShaderApi.h"
#include <memory>

class ProgramApi
{
public:
    virtual std::unique_ptr<ShaderApi> CreateShader(ShaderType type) = 0;
    virtual void UseProgram() = 0;
};
