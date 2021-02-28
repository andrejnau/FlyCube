#pragma once
#include "Shader/Shader.h"
#include <Instance/BaseTypes.h>
#include <ShaderReflection/ShaderReflection.h>
#include <map>

class ShaderBase : public Shader
{
public:
    ShaderBase(const ShaderDesc& desc, ShaderBlobType blob_type);
    ResourceBindingDesc GetResourceBindingDesc(const BindKey& bind_key) const override;
    ShaderType GetType() const override;
    BindKey GetBindKey(const std::string& name) const override;
    const std::vector<uint8_t>& GetBlob() const override;

protected:
    ShaderType m_shader_type;
    ShaderBlobType m_blob_type;
    std::vector<uint8_t> m_blob;
    std::vector<ResourceBindingDesc> m_bindings;
    std::map<BindKey, size_t> m_mapping;
    std::map<std::string, BindKey> m_bind_keys;
};
