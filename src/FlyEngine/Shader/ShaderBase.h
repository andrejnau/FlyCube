#pragma once

#include "Shader/ShaderDesc.h"

#include <Context/BaseTypes.h>
#include <string>
#include <map>
#include <vector>

class ShaderBase : public ShaderDesc
{
public:
    ShaderType type;

    virtual ~ShaderBase() = default;
    virtual void UpdateShader() = 0;

    ShaderBase(ShaderType type, const std::string& shader_path, const std::string& entrypoint, const std::string& target)
        : ShaderDesc(shader_path, entrypoint, target)
        , type(type)
    {
    }
};
