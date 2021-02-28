#include "Shader/SpirvShader.h"
#include <spirv_hlsl.hpp>

SpirvShader::SpirvShader(const ShaderDesc& desc)
    : ShaderBase(desc, ShaderBlobType::kSPIRV)
{
    if (m_shader_type == ShaderType::kVertex)
        ParseInputLayout();
}

std::vector<VertexInputDesc> SpirvShader::GetInputLayout() const
{
    return m_input_layout_desc;
}

void SpirvShader::ParseInputLayout()
{
    spirv_cross::CompilerHLSL compiler((uint32_t*)m_blob.data(), m_blob.size() / sizeof(uint32_t));
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();
    for (auto& resource : resources.stage_inputs)
    {
        decltype(auto) layout = m_input_layout_desc.emplace_back();
        layout.slot = layout.location = compiler.get_decoration(resource.id, spv::DecorationLocation);
        std::string semantic = compiler.get_decoration_string(resource.id, spv::DecorationHlslSemanticGOOGLE);
        m_locations[semantic] = layout.location;
        decltype(auto) type = compiler.get_type(resource.base_type_id);

        if (type.basetype == spirv_cross::SPIRType::Float)
        {
            if (type.vecsize == 1)
                layout.format = gli::format::FORMAT_R32_SFLOAT_PACK32;
            else if (type.vecsize == 2)
                layout.format = gli::format::FORMAT_RG32_SFLOAT_PACK32;
            else if (type.vecsize == 3)
                layout.format = gli::format::FORMAT_RGB32_SFLOAT_PACK32;
            else if (type.vecsize == 4)
                layout.format = gli::format::FORMAT_RGBA32_SFLOAT_PACK32;
        }
        else if (type.basetype == spirv_cross::SPIRType::UInt)
        {
            if (type.vecsize == 1)
                layout.format = gli::format::FORMAT_R32_UINT_PACK32;
            else if (type.vecsize == 2)
                layout.format = gli::format::FORMAT_RG32_UINT_PACK32;
            else if (type.vecsize == 3)
                layout.format = gli::format::FORMAT_RGB32_UINT_PACK32;
            else if (type.vecsize == 4)
                layout.format = gli::format::FORMAT_RGBA32_UINT_PACK32;
        }
        else if (type.basetype == spirv_cross::SPIRType::Int)
        {
            if (type.vecsize == 1)
                layout.format = gli::format::FORMAT_R32_SINT_PACK32;
            else if (type.vecsize == 2)
                layout.format = gli::format::FORMAT_RG32_SINT_PACK32;
            else if (type.vecsize == 3)
                layout.format = gli::format::FORMAT_RGB32_SINT_PACK32;
            else if (type.vecsize == 4)
                layout.format = gli::format::FORMAT_RGBA32_SINT_PACK32;
        }
        layout.stride = gli::detail::bits_per_pixel(layout.format) / 8;
    }
}

uint32_t SpirvShader::GetResourceStride(const BindKey& bind_key) const
{
    return 0;
}

uint32_t SpirvShader::GetVertexInputLocation(const std::string& semantic_name) const
{
    return m_locations.at(semantic_name);
}
