#include "Shader/ShaderBase.h"
#include <HLSLCompiler/Compiler.h>

ShaderBase::ShaderBase(const ShaderDesc& desc, ShaderBlobType blob_type)
    : m_shader_type(desc.type)
    , m_blob_type(blob_type)
{
    m_blob = Compile(desc, blob_type);
    std::shared_ptr<ShaderReflection> reflection = CreateShaderReflection(blob_type, m_blob.data(), m_blob.size());
    m_bindings = reflection->GetBindings();
    for (uint32_t i = 0; i < m_bindings.size(); ++i)
    {
        BindKey bind_key = { m_shader_type, m_bindings[i].type, m_bindings[i].slot, m_bindings[i].space };
        m_bind_keys[m_bindings[i].name] = bind_key;
        m_mapping[bind_key] = i;
    }
}

ResourceBindingDesc ShaderBase::GetResourceBindingDesc(const BindKey& bind_key) const
{
    return m_bindings.at(m_mapping.at(bind_key));
}

ShaderType ShaderBase::GetType() const
{
    return m_shader_type;
}

BindKey ShaderBase::GetBindKey(const std::string& name) const
{
    auto it = m_bind_keys.find(name);
    if (it != m_bind_keys.end())
        return it->second;
    assert(m_blob_type == ShaderBlobType::kSPIRV);
    return m_bind_keys.at("type_" + name);
}

const std::vector<uint8_t>& ShaderBase::GetBlob() const
{
    return m_blob;
}
