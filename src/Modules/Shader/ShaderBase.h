#pragma once

#include <Context/BaseTypes.h>
#include <string>
#include <map>
#include <vector>

class ShaderBase
{
protected:
    
public:
    virtual ~ShaderBase() = default;
    virtual void UpdateShader() = 0;

    ShaderBase(ShaderType type, const std::string& shader_path, const std::string& entrypoint, const std::string& target)
        : type(type)
        , shader_path(shader_path)
        , entrypoint(entrypoint)
        , target(target)
    {
    }

    ShaderType type;
    std::string shader_path;
    std::string entrypoint;
    std::string target;
    std::map<std::string, std::string> define;
};
