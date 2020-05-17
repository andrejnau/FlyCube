#pragma once

#include <Instance/BaseTypes.h>
#include <string>
#include <map>
#include <vector>

class ShaderBase : public ShaderDesc
{
public:
    virtual ~ShaderBase() = default;
    virtual void UpdateShader() = 0;

    ShaderBase(ShaderType type, const std::string& shader_path, const std::string& entrypoint, const std::string& target)
        : ShaderDesc(shader_path, entrypoint, GetShaderType(target))
    {
    }

    static ShaderType GetShaderType(const std::string& target)
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
