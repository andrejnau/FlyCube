#include "ShaderReflection/SPIRVReflection.h"

ShaderKind ConvertShaderKind(spv::ExecutionModel execution_model)
{
    switch (execution_model)
    {
    case spv::ExecutionModel::ExecutionModelVertex:
        return ShaderKind::kVertex;
    case spv::ExecutionModel::ExecutionModelFragment:
        return ShaderKind::kPixel;
    case spv::ExecutionModel::ExecutionModelGeometry:
        return ShaderKind::kGeometry;
    case spv::ExecutionModel::ExecutionModelGLCompute:
        return ShaderKind::kCompute;
    case spv::ExecutionModel::ExecutionModelRayGenerationNV:
        return ShaderKind::kRayGeneration;
    case spv::ExecutionModel::ExecutionModelIntersectionNV:
        return ShaderKind::kIntersection;
    case spv::ExecutionModel::ExecutionModelAnyHitNV:
        return ShaderKind::kAnyHit;
    case spv::ExecutionModel::ExecutionModelClosestHitNV:
        return ShaderKind::kClosestHit;
    case spv::ExecutionModel::ExecutionModelMissNV:
        return ShaderKind::kMiss;
    case spv::ExecutionModel::ExecutionModelCallableNV:
        return ShaderKind::kCallable;
    case spv::ExecutionModel::ExecutionModelTaskNV:
        return ShaderKind::kAmplification;
    case spv::ExecutionModel::ExecutionModelMeshNV:
        return ShaderKind::kMesh;
    }
    assert(false);
    return ShaderKind::kUnknown;
}

std::vector<InputParameterDesc> ParseInputParameters(const spirv_cross::Compiler& compiler)
{
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();
    std::vector<InputParameterDesc> input_parameters;
    for (const auto& resource : resources.stage_inputs)
    {
        decltype(auto) input = input_parameters.emplace_back();
        input.location = compiler.get_decoration(resource.id, spv::DecorationLocation);
        input.semantic_name = compiler.get_decoration_string(resource.id, spv::DecorationHlslSemanticGOOGLE);
        decltype(auto) type = compiler.get_type(resource.base_type_id);
        if (type.basetype == spirv_cross::SPIRType::Float)
        {
            if (type.vecsize == 1)
                input.format = gli::format::FORMAT_R32_SFLOAT_PACK32;
            else if (type.vecsize == 2)
                input.format = gli::format::FORMAT_RG32_SFLOAT_PACK32;
            else if (type.vecsize == 3)
                input.format = gli::format::FORMAT_RGB32_SFLOAT_PACK32;
            else if (type.vecsize == 4)
                input.format = gli::format::FORMAT_RGBA32_SFLOAT_PACK32;
        }
        else if (type.basetype == spirv_cross::SPIRType::UInt)
        {
            if (type.vecsize == 1)
                input.format = gli::format::FORMAT_R32_UINT_PACK32;
            else if (type.vecsize == 2)
                input.format = gli::format::FORMAT_RG32_UINT_PACK32;
            else if (type.vecsize == 3)
                input.format = gli::format::FORMAT_RGB32_UINT_PACK32;
            else if (type.vecsize == 4)
                input.format = gli::format::FORMAT_RGBA32_UINT_PACK32;
        }
        else if (type.basetype == spirv_cross::SPIRType::Int)
        {
            if (type.vecsize == 1)
                input.format = gli::format::FORMAT_R32_SINT_PACK32;
            else if (type.vecsize == 2)
                input.format = gli::format::FORMAT_RG32_SINT_PACK32;
            else if (type.vecsize == 3)
                input.format = gli::format::FORMAT_RGB32_SINT_PACK32;
            else if (type.vecsize == 4)
                input.format = gli::format::FORMAT_RGBA32_SINT_PACK32;
        }
    }
    return input_parameters;
}

bool IsBufferDimension(spv::Dim dimension)
{
    switch (dimension)
    {
    case spv::Dim::DimBuffer:
        return true;
    case spv::Dim::Dim1D:
    case spv::Dim::Dim2D:
    case spv::Dim::Dim3D:
    case spv::Dim::DimCube:
        return false;
    default:
        assert(false);
        return false;
    }
}

ViewType GetViewType(const spirv_cross::Compiler& compiler, const spirv_cross::SPIRType& type, uint32_t resource_id)
{
    switch (type.basetype)
    {
    case spirv_cross::SPIRType::AccelerationStructure:
    {
        return ViewType::kAccelerationStructure;
    }
    case spirv_cross::SPIRType::SampledImage:
    case spirv_cross::SPIRType::Image:
    {
        bool is_readonly = (type.image.sampled != 2);
        if (IsBufferDimension(type.image.dim))
        {
            if (is_readonly)
                return ViewType::kBuffer;
            else
                return ViewType::kRWBuffer;
        }
        else
        {
            if (is_readonly)
                return ViewType::kTexture;
            else
                return ViewType::kRWTexture;
        }
    }
    case spirv_cross::SPIRType::Sampler:
    {
        return ViewType::kSampler;
    }
    case spirv_cross::SPIRType::Struct:
    {
        if (type.storage == spv::StorageClassStorageBuffer)
        {
            spirv_cross::Bitset flags = compiler.get_buffer_block_flags(resource_id);
            bool is_readonly = flags.get(spv::DecorationNonWritable);
            if (is_readonly)
            {
                return ViewType::kStructuredBuffer;
            }
            else
            {
                return ViewType::kRWStructuredBuffer;
            }
        }
        else if (type.storage == spv::StorageClassPushConstant || type.storage == spv::StorageClassUniform)
        {
            return ViewType::kConstantBuffer;
        }
        assert(false);
        return ViewType::kUnknown;
    }
    default:
        assert(false);
        return ViewType::kUnknown;
    }
}

ViewDimension GetDimension(spv::Dim dim, const spirv_cross::SPIRType& resource_type)
{
    switch (dim)
    {
    case spv::Dim::Dim1D:
    {
        if (resource_type.image.arrayed)
            return ViewDimension::kTexture1DArray;
        else
            return ViewDimension::kTexture1D;
    }
    case spv::Dim::Dim2D:
    {
        if (resource_type.image.arrayed)
            return ViewDimension::kTexture2DArray;
        else
            return ViewDimension::kTexture2D;
    }
    case spv::Dim::Dim3D:
    {
        return ViewDimension::kTexture3D;
    }
    case spv::Dim::DimCube:
    {
        if (resource_type.image.arrayed)
            return ViewDimension::kTextureCubeArray;
        else
            return ViewDimension::kTextureCube;
    }
    case spv::Dim::DimBuffer:
    {
        return ViewDimension::kBuffer;
    }
    default:
        assert(false);
        return ViewDimension::kUnknown;
    }
}

ViewDimension GetViewDimension(const spirv_cross::SPIRType& resource_type)
{
    if (resource_type.basetype == spirv_cross::SPIRType::BaseType::Image)
    {
        return GetDimension(resource_type.image.dim, resource_type);
    }
    else if (resource_type.basetype == spirv_cross::SPIRType::BaseType::Struct)
    {
        return ViewDimension::kBuffer;
    }
    else
    {
        return ViewDimension::kUnknown;
    }
}

ReturnType GetReturnType(const spirv_cross::CompilerHLSL& compiler, const spirv_cross::SPIRType& resource_type)
{
    if (resource_type.basetype == spirv_cross::SPIRType::BaseType::Image)
    {
        decltype(auto) image_type = compiler.get_type(resource_type.image.type);
        switch (image_type.basetype)
        {
        case spirv_cross::SPIRType::BaseType::Float:
            return ReturnType::kFloat;
        case spirv_cross::SPIRType::BaseType::UInt:
            return ReturnType::kUint;
        case spirv_cross::SPIRType::BaseType::Int:
            return ReturnType::kInt;
        case spirv_cross::SPIRType::BaseType::Double:
            return ReturnType::kDouble;
        }
        assert(false);
    }
    return ReturnType::kUnknown;
}

ResourceBindingDesc GetBindingDesc(const spirv_cross::CompilerHLSL& compiler, const spirv_cross::Resource& resource)
{
    ResourceBindingDesc desc = {};
    decltype(auto) resource_type = compiler.get_type(resource.type_id);
    desc.name = compiler.get_name(resource.id);
    desc.type = GetViewType(compiler, resource_type, resource.id);
    desc.slot = compiler.get_decoration(resource.id, spv::DecorationBinding);
    desc.space = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
    desc.count = 1;
    if (!resource_type.array.empty() && resource_type.array.front() == 0)
    {
        desc.count = std::numeric_limits<uint32_t>::max();
    }
    desc.dimension = GetViewDimension(resource_type);
    desc.return_type = GetReturnType(compiler, resource_type);
    switch (desc.type)
    {
    case ViewType::kStructuredBuffer:
    case ViewType::kRWStructuredBuffer:
    {
        bool is_block = compiler.get_decoration_bitset(resource_type.self).get(spv::DecorationBlock) ||
                        compiler.get_decoration_bitset(resource_type.self).get(spv::DecorationBufferBlock);
        bool is_sized_block = is_block && (compiler.get_storage_class(resource.id) == spv::StorageClassUniform ||
                                           compiler.get_storage_class(resource.id) == spv::StorageClassUniformConstant ||
                                           compiler.get_storage_class(resource.id) == spv::StorageClassStorageBuffer);
        assert(is_sized_block);
        decltype(auto) base_type = compiler.get_type(resource.base_type_id);
        desc.structure_stride = compiler.get_declared_struct_size_runtime_array(base_type, 1) - compiler.get_declared_struct_size_runtime_array(base_type, 0);
        assert(desc.structure_stride);
        break;
    }
    }
    return desc;
}

std::vector<ResourceBindingDesc> ParseBindings(const spirv_cross::CompilerHLSL& compiler)
{
    std::vector<ResourceBindingDesc> bindings;
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();
    auto enumerate_resources = [&](const spirv_cross::SmallVector<spirv_cross::Resource>& resources)
    {
        for (const auto& resource : resources)
        {
            bindings.emplace_back(GetBindingDesc(compiler, resource));
        }
    };
    enumerate_resources(resources.uniform_buffers);
    enumerate_resources(resources.storage_buffers);
    enumerate_resources(resources.storage_images);
    enumerate_resources(resources.separate_images);
    enumerate_resources(resources.separate_samplers);
    enumerate_resources(resources.atomic_counters);
    enumerate_resources(resources.acceleration_structures);
    return bindings;
}

SPIRVReflection::SPIRVReflection(const void* data, size_t size)
    : m_blob((const uint32_t*)data, (const uint32_t*)data + size / sizeof(uint32_t))
{
    spirv_cross::CompilerHLSL compiler(m_blob);
    auto entry_points = compiler.get_entry_points_and_stages();
    for (const auto& entry_point : entry_points)
    {
        m_entry_points.push_back({ entry_point.name.c_str(), ConvertShaderKind(entry_point.execution_model) });
    }
    m_bindings = ParseBindings(compiler);
    m_input_parameters = ParseInputParameters(compiler);
}

const std::vector<EntryPoint> SPIRVReflection::GetEntryPoints() const
{
    return m_entry_points;
}

const std::vector<ResourceBindingDesc> SPIRVReflection::GetBindings() const
{
    return m_bindings;
}

const std::vector<InputParameterDesc> SPIRVReflection::GetInputParameters() const
{
    return m_input_parameters;
}
