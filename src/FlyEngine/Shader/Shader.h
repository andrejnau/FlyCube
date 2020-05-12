#pragma once
#include <Instance/BaseTypes.h>
#include <memory>

class Shader
{
public:
    virtual ~Shader() = default;
    virtual ShaderType GetType() const = 0;
};
