#pragma once

#include <Context/BaseTypes.h>
#include <string>
#include <map>
#include <cassert>

struct ShaderDesc
{
    std::string shader_path;
    std::string entrypoint;
    std::string target;
    std::map<std::string, std::string> define;
    ShaderType type;

    ShaderDesc(const std::string& shader_path, const std::string& entrypoint, const std::string& target)
        : shader_path(shader_path)
        , entrypoint(entrypoint)
        , target(target)
        , type(GetShaderType())
    {
    }

private:
    ShaderType GetShaderType()
    {
        if (target.find("ps") == 0)
            return ShaderType::kPixel;
        else if (target.find("vs") == 0)
            return ShaderType::kVertex;
        else if (target.find("cs") == 0)
            return ShaderType::kCompute;
        else if (target.find("gs") == 0)
            return ShaderType::kGeometry;
        else if (target.find("lib") == 0)
            return ShaderType::kLibrary;
        assert(false);
        return ShaderType::kUnknown;
    }
};
