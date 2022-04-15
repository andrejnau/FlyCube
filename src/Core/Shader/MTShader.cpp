#include "Shader/MTShader.h"
#include <HLSLCompiler/MSLConverter.h>

MTShader::MTShader(const ShaderDesc& desc, ShaderBlobType blob_type)
    : ShaderBase(desc, blob_type)
{
    m_source = GetMSLShader(m_blob, m_index_mapping);
}

const std::string& MTShader::GetSource() const
{
    return m_source;
}

uint32_t MTShader::GetIndex(BindKey bind_key) const
{
    return m_index_mapping.at(m_bindings.at(m_mapping.at(bind_key)).name);
}
