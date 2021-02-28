#pragma once
#include <Instance/QueryInterface.h>
#include <Instance/BaseTypes.h>
#include <memory>

class Shader : public QueryInterface
{
public:
    virtual ~Shader() = default;
    virtual ShaderType GetType() const = 0;
    virtual const std::vector<uint8_t>& GetBlob() const = 0;
    virtual const BindKey& GetBindKey(const std::string& name) const = 0;
    virtual const std::vector<ResourceBindingDesc>& GetResourceBindings() const = 0;
    virtual const ResourceBindingDesc& GetResourceBinding(const BindKey& bind_key) const = 0;
    virtual const std::vector<InputLayoutDesc>& GetInputLayouts() const = 0;
    virtual uint32_t GetInputLayoutLocation(const std::string& semantic_name) const = 0;
    virtual const std::string& GetSemanticName(uint32_t location) const = 0;
};
