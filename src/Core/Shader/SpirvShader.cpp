#include "Shader/SpirvShader.h"
#include <HLSLCompiler/SpirvCompiler.h>
#include <spirv_hlsl.hpp>

ViewType GetViewType(const spirv_cross::Compiler& compiler, const spirv_cross::SPIRType& type, uint32_t resource_id)
{
    switch (type.basetype)
    {
    case spirv_cross::SPIRType::SampledImage:
    {
        return ViewType::kShaderResource;
    }
    case spirv_cross::SPIRType::Image:
    {
        if (type.image.sampled == 2 && type.image.dim != spv::DimSubpassData)
            return ViewType::kUnorderedAccess;
        else
            return ViewType::kShaderResource;
    }
    case spirv_cross::SPIRType::Sampler:
    {
        return ViewType::kSampler;
    }
    case spirv_cross::SPIRType::Struct:
    {
        if (type.storage == spv::StorageClassUniform || type.storage == spv::StorageClassStorageBuffer)
        {
            if (compiler.has_decoration(type.self, spv::DecorationBufferBlock))
            {
                spirv_cross::Bitset flags = compiler.get_buffer_block_flags(resource_id);
                bool is_readonly = flags.get(spv::DecorationNonWritable);
                if (is_readonly)
                    return ViewType::kShaderResource;
                else
                    return ViewType::kUnorderedAccess;
            }
            else if (compiler.has_decoration(type.self, spv::DecorationBlock))
            {
                return ViewType::kConstantBuffer;
            }
        }
        else if (type.storage == spv::StorageClassPushConstant)
        {
            return ViewType::kConstantBuffer;
        }
        else
        {
            return ViewType::kUnknown;
        }
    }
    default:
        return ViewType::kUnknown;
    }
}

SpirvShader::SpirvShader(const ShaderDesc& desc)
    : m_type(desc.type)
{
    SpirvOption option = {};
    option.auto_map_bindings = true;
    option.hlsl_iomap = true;
    option.resource_set_binding = static_cast<uint32_t>(desc.type);
    m_blob = SpirvCompile(desc, option);

    spirv_cross::CompilerHLSL compiler(m_blob);
    spirv_cross::ShaderResources shader_resources = compiler.get_shader_resources();

    auto add_resources = [&](const spirv_cross::SmallVector<spirv_cross::Resource>& resources)
    {
        for (const auto& resource : resources)
        {
            auto& res_type = compiler.get_type(resource.type_id);
            auto slot = compiler.get_decoration(resource.id, spv::DecorationBinding);
            auto space = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);

            if (space == 0)
            {
                uint32_t word_offset = 0;
                compiler.get_binary_offset_for_decoration(resource.id, spv::DecorationDescriptorSet, word_offset);
                m_blob[word_offset] = space = option.resource_set_binding;
            }

            ViewType view_type = GetViewType(compiler, res_type, resource.id);
            BindKey bind_key = { m_type, view_type, slot, space };
            m_bind_keys[resource.name] = bind_key;
            auto& resource_binding_desc = m_resource_binding_descs[bind_key];
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

    add_resources(shader_resources.uniform_buffers);
    add_resources(shader_resources.storage_buffers);
    add_resources(shader_resources.storage_images);
    add_resources(shader_resources.atomic_counters);
    add_resources(shader_resources.acceleration_structures);
    add_resources(shader_resources.separate_images);
    add_resources(shader_resources.separate_samplers);

    if (m_type == ShaderType::kVertex)
        m_input_layout_desc = GetInputLayoutImpl();
}

std::vector<VertexInputDesc> SpirvShader::GetInputLayout() const
{
    return m_input_layout_desc;
}

std::vector<VertexInputDesc> SpirvShader::GetInputLayoutImpl() const
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

ResourceBindingDesc SpirvShader::GetResourceBindingDesc(const BindKey& bind_key) const
{
    return m_resource_binding_descs.at(bind_key);
}

uint32_t SpirvShader::GetResourceStride(const BindKey& bind_key) const
{
    return 0;
}

ShaderType SpirvShader::GetType() const
{
    return m_type;
}

BindKey SpirvShader::GetBindKey(const std::string& name) const
{
    auto it = m_bind_keys.find(name);
    if (it != m_bind_keys.end())
        return it->second;
    return m_bind_keys.at("type_" + name);
}

const std::vector<uint32_t>& SpirvShader::GetBlob() const
{
    return m_blob;
}
