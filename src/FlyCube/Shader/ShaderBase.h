#pragma once
#include "Instance/BaseTypes.h"
#include "Shader/Shader.h"
#include "ShaderReflection/ShaderReflection.h"

#include <map>

class ShaderBase : public Shader {
public:
    ShaderBase(const std::vector<uint8_t>& blob, ShaderBlobType blob_type, ShaderType shader_type, bool is_msl = false);
    ShaderBase(const ShaderDesc& desc, ShaderBlobType blob_type, bool is_msl = false);
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
    std::vector<uint8_t> m_blob;
    ShaderBlobType m_blob_type;
    ShaderType m_shader_type;
    std::map<std::string, uint64_t> m_ids;
    std::vector<ResourceBindingDesc> m_bindings;
    std::vector<BindKey> m_binding_keys;
    std::map<BindKey, size_t> m_mapping;
    std::map<std::string, BindKey> m_bind_keys;
    std::vector<InputLayoutDesc> m_input_layout_descs;
    std::map<std::string, uint32_t> m_locations;
    std::shared_ptr<ShaderReflection> m_reflection;
    std::map<std::string, uint32_t> m_slot_remapping;
    std::string m_msl_source;
};
