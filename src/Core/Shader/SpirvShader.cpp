#include "Shader/SpirvShader.h"
#include "Shader/SpirvCompiler.h"
#include <spirv_hlsl.hpp>

SpirvShader::SpirvShader(const ShaderDesc& desc)
    : m_type(desc.type)
{
    SpirvOption option = {};
    option.auto_map_bindings = true;
    option.hlsl_iomap = true;
    option.invert_y = !desc.define.count("__INTERNAL_DO_NOT_INVERT_Y__");
    if (desc.type == ShaderType::kLibrary)
        option.use_dxc = true;
    option.resource_set_binding = static_cast<uint32_t>(desc.type);
    m_blob = SpirvCompile(desc, option);

    spirv_cross::CompilerHLSL compiler(m_blob);
    spirv_cross::ShaderResources shader_resources = compiler.get_shader_resources();

    auto add_resources = [&](const spirv_cross::SmallVector<spirv_cross::Resource>& resources)
    {
        for (const auto& resource : resources)
        {
            auto& resource_binding_desc = m_resource_binding_descs[compiler.get_name(resource.id)];
            auto& res_type = compiler.get_type(resource.type_id);
            auto& dim = res_type.image.dim;

            if (res_type.basetype == spirv_cross::SPIRType::BaseType::Struct)
            {
                resource_binding_desc.dimension = ResourceDimension::kBuffer;
                continue;
            }
            if (res_type.basetype == spirv_cross::SPIRType::BaseType::AccelerationStructure)
            {
                resource_binding_desc.dimension = ResourceDimension::kRaytracingAccelerationStructure;
                continue;
            }

            switch (dim)
            {
            case spv::Dim::Dim1D:
            {
                if (res_type.image.arrayed)
                    resource_binding_desc.dimension = ResourceDimension::kTexture1DArray;
                else
                    resource_binding_desc.dimension = ResourceDimension::kTexture1D;
                break;
            }
            case spv::Dim::Dim2D:
            {
                if (res_type.image.arrayed)
                    resource_binding_desc.dimension = ResourceDimension::kTexture2DArray;
                else
                    resource_binding_desc.dimension = ResourceDimension::kTexture2D;
                break;
            }
            case spv::Dim::Dim3D:
            {
                resource_binding_desc.dimension = ResourceDimension::kTexture3D;
                break;
            }
            case spv::Dim::DimCube:
            {
                if (res_type.image.arrayed)
                    resource_binding_desc.dimension = ResourceDimension::kTextureCubeArray;
                else
                    resource_binding_desc.dimension = ResourceDimension::kTextureCube;
                break;
            }
            default:
            {
                break;
            }
            }
        }
    };

    add_resources(shader_resources.separate_images);
    add_resources(shader_resources.storage_images);
    add_resources(shader_resources.storage_buffers);
    add_resources(shader_resources.acceleration_structures);
}

std::vector<VertexInputDesc> SpirvShader::GetInputLayout() const
{
    spirv_cross::CompilerHLSL compiler(m_blob);
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();
    std::vector<VertexInputDesc> input_layout_desc;
    for (auto& resource : resources.stage_inputs)
    {
        decltype(auto) layout = input_layout_desc.emplace_back();
        layout.slot = compiler.get_decoration(resource.id, spv::DecorationLocation);
        layout.semantic_name = compiler.get_decoration_string(resource.id, spv::DecorationHlslSemanticGOOGLE);
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
    return input_layout_desc;
}

ResourceBindingDesc SpirvShader::GetResourceBindingDesc(const std::string& name) const
{
    return m_resource_binding_descs.at(name);
}

uint32_t SpirvShader::GetResourceStride(const std::string& name) const
{
    return 0;
}

ShaderType SpirvShader::GetType() const
{
    return m_type;
}

const std::vector<uint32_t>& SpirvShader::GetBlob() const
{
    return m_blob;
}
