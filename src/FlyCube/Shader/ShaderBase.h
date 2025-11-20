#pragma once
#include "Instance/BaseTypes.h"
#include "Shader/Shader.h"
#include "ShaderReflection/ShaderReflection.h"

#include <map>
#include <vector>

class ShaderBase : public Shader {
public:
    ShaderBase(const std::vector<uint8_t>& blob, ShaderBlobType blob_type, ShaderType shader_type);
    ShaderType GetType() const override;
    const std::vector<uint8_t>& GetBlob() const override;
    uint64_t GetId(const std::string& entry_point) const override;
    const BindKey& GetBindKey(const std::string& name) const override;
    const std::vector<ResourceBindingDesc>& GetResourceBindings() const override;
    const ResourceBindingDesc& GetResourceBinding(const BindKey& bind_key) const override;
    const std::vector<InputLayoutDesc>& GetInputLayouts() const override;
    uint32_t GetInputLayoutLocation(const std::string& semantic_name) const override;
    const std::vector<BindKey>& GetBindings() const override;
    const std::shared_ptr<ShaderReflection>& GetReflection() const override;

protected:
    std::vector<uint8_t> blob_;
    ShaderBlobType blob_type_;
    ShaderType shader_type_;
    std::map<std::string, uint64_t> ids_;
    std::vector<ResourceBindingDesc> bindings_;
    std::vector<BindKey> binding_keys_;
    std::map<BindKey, size_t> mapping_;
    std::map<std::string, BindKey> bind_keys_;
    std::vector<InputLayoutDesc> input_layout_descs_;
    std::map<std::string, uint32_t> locations_;
    std::shared_ptr<ShaderReflection> reflection_;
};
