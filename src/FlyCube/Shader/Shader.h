#pragma once
#include "Instance/BaseTypes.h"
#include "Instance/QueryInterface.h"
#include "ShaderReflection/ShaderReflection.h"

#include <memory>

class Shader : public QueryInterface {
public:
    virtual ~Shader() = default;
    virtual ShaderType GetType() const = 0;
    virtual const std::vector<uint8_t>& GetBlob() const = 0;
    virtual uint64_t GetId(const std::string& entry_point) const = 0;
    virtual const BindKey& GetBindKey(const std::string& name) const = 0;
    virtual const std::vector<ResourceBindingDesc>& GetResourceBindings() const = 0;
    virtual const ResourceBindingDesc& GetResourceBinding(const BindKey& bind_key) const = 0;
    virtual const std::vector<InputLayoutDesc>& GetInputLayouts() const = 0;
    virtual uint32_t GetInputLayoutLocation(const std::string& semantic_name) const = 0;
    virtual const std::vector<BindKey>& GetBindings() const = 0;
    virtual const std::shared_ptr<ShaderReflection>& GetReflection() const = 0;
};
