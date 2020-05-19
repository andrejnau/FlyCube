#pragma once
#include <Instance/QueryInterface.h>
#include <Instance/BaseTypes.h>
#include <memory>

class Shader : public QueryInterface
{
public:
    virtual ~Shader() = default;
    virtual std::vector<VertexInputDesc> GetInputLayout() const = 0;
    virtual ShaderType GetType() const = 0;
};
