#pragma once
#include <Instance/QueryInterface.h>
#include <Instance/BaseTypes.h>
#include <memory>

class Shader : public QueryInterface
{
public:
    virtual ~Shader() = default;
    virtual std::vector<VertexInputDesc> GetInputLayout() const = 0;
    virtual ResourceBindingDesc GetResourceBindingDesc(const BindKey& bind_key) const = 0;
    virtual uint32_t GetResourceStride(const BindKey& bind_key) const = 0;
    virtual ShaderType GetType() const = 0;
    virtual BindKey GetBindKey(const std::string& name) const = 0;
    virtual uint32_t GetVertexInputLocation(const std::string& semantic_name) const = 0;
    virtual const std::vector<uint8_t>& GetBlob() const = 0;
};
