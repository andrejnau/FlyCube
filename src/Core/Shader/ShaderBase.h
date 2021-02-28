#pragma once
#include "Shader/Shader.h"
#include <Instance/BaseTypes.h>
#include <ShaderReflection/ShaderReflection.h>
#include <map>

class ShaderBase : public Shader
{
public:
    ShaderBase(const ShaderDesc& desc, ShaderBlobType blob_type);
    ShaderType GetType() const override;
    const std::vector<uint8_t>& GetBlob() const override;
    const BindKey& GetBindKey(const std::string& name) const override;
    const std::vector<ResourceBindingDesc>& GetResourceBindings() const override;
    const ResourceBindingDesc& GetResourceBinding(const BindKey& bind_key) const override;
    const std::vector<InputLayoutDesc>& GetInputLayouts() const override;
    uint32_t GetInputLayoutLocation(const std::string& semantic_name) const override;
    const std::string& GetSemanticName(uint32_t location) const override;

protected:
    ShaderType m_shader_type;
    ShaderBlobType m_blob_type;
    std::vector<uint8_t> m_blob;
    std::vector<ResourceBindingDesc> m_bindings;
    std::map<BindKey, size_t> m_mapping;
    std::map<std::string, BindKey> m_bind_keys;
    std::vector<InputLayoutDesc> m_input_layout_descs;
    std::map<std::string, uint32_t> m_locations;
    std::map<uint32_t, std::string> m_semantic_names;
};
