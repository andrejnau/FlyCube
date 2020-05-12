#include "Shader/DXShader.h"
#include "Shader/DXCompiler.h"

DXShader::DXShader(const ShaderDesc& desc)
    : m_type(desc.type)
{
    m_blob = DXCompile(desc);
}

ShaderType DXShader::GetType() const
{
    return m_type;
}

ComPtr<ID3DBlob> DXShader::GetBlob() const
{
    return m_blob;
}
