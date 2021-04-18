#pragma once
#include <string>
#include <ShaderReflection/ShaderReflection.h>

struct Option
{
    std::string mustache_template;
    std::string shader_name;
    std::string assets_path;
    std::string shader_path;
    std::string entrypoint;
    ShaderType type;
    std::string model;
    ShaderBlobType blob_type;
};

std::string RenderShaderReflection(const Option& option);
