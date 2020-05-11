#include "Shader/DXShader.h"
#include "Shader/DXCompiler.h"

DXShader::DXShader(const ShaderDesc& desc)
{
    m_blob = DXCompile(desc);
}
