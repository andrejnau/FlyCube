#pragma once

#include <Context/BaseTypes.h>
#include <string>
#include <map>

struct ShaderDesc
{
    std::string shader_path;
    std::string entrypoint;
    std::string target;
    std::map<std::string, std::string> define;

    ShaderDesc(const std::string& shader_path, const std::string& entrypoint, const std::string& target)
        : shader_path(shader_path)
        , entrypoint(entrypoint)
        , target(target)
    {
    }
};
