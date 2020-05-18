#include "Shader/SpirvShader.h"
#include "Shader/SpirvCompiler.h"

SpirvShader::SpirvShader(const ShaderDesc& desc)
    : m_type(desc.type)
{
    SpirvOption option = {};
    option.auto_map_bindings = true;
    option.hlsl_iomap = true;
    option.invert_y = true;
    if (desc.type == ShaderType::kLibrary)
        option.use_dxc = true;
    option.resource_set_binding = static_cast<uint32_t>(desc.type);
    m_blob = SpirvCompile(desc, option);
}

std::vector<VertexInputDesc> SpirvShader::GetInputLayout() const
{
    return {};
}

std::vector<RenderTargetDesc> SpirvShader::GetRenderTargets() const
{
    return {};
}

ShaderType SpirvShader::GetType() const
{
    return m_type;
}

const std::vector<uint32_t>& SpirvShader::GetBlob() const
{
    return m_blob;
}
