#include "Shader/SpirvShader.h"
#include "Shader/SpirvCompiler.h"

SpirvShader::SpirvShader(const ShaderDesc& desc)
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
