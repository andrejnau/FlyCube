#include "PipelineProgram/VKPipelineProgram.h"
#include <Device/VKDevice.h>
#include <Shader/SpirvShader.h>

vk::ShaderStageFlagBits ShaderType2Bit(ShaderType type)
{
    switch (type)
    {
    case ShaderType::kVertex:
        return vk::ShaderStageFlagBits::eVertex;
    case ShaderType::kPixel:
        return vk::ShaderStageFlagBits::eFragment;
    case ShaderType::kGeometry:
        return vk::ShaderStageFlagBits::eGeometry;
    case ShaderType::kCompute:
        return vk::ShaderStageFlagBits::eCompute;
    case ShaderType::kLibrary:
        return vk::ShaderStageFlagBits::eAll;
    }
    return {};
}

vk::ShaderStageFlagBits ExecutionModel2Bit(spv::ExecutionModel model)
{
    switch (model)
    {
    case spv::ExecutionModel::ExecutionModelVertex:
        return vk::ShaderStageFlagBits::eVertex;
    case spv::ExecutionModel::ExecutionModelFragment:
        return vk::ShaderStageFlagBits::eFragment;
    case spv::ExecutionModel::ExecutionModelGeometry:
        return vk::ShaderStageFlagBits::eGeometry;
    case spv::ExecutionModel::ExecutionModelGLCompute:
        return vk::ShaderStageFlagBits::eCompute;
    case spv::ExecutionModel::ExecutionModelRayGenerationNV:
        return vk::ShaderStageFlagBits::eRaygenNV;
    case spv::ExecutionModel::ExecutionModelIntersectionNV:
        return vk::ShaderStageFlagBits::eIntersectionNV;
    case spv::ExecutionModel::ExecutionModelAnyHitNV:
        return vk::ShaderStageFlagBits::eAnyHitNV;
    case spv::ExecutionModel::ExecutionModelClosestHitNV:
        return vk::ShaderStageFlagBits::eClosestHitNV;
    case spv::ExecutionModel::ExecutionModelMissNV:
        return vk::ShaderStageFlagBits::eMissNV;
    case spv::ExecutionModel::ExecutionModelCallableNV:
        return vk::ShaderStageFlagBits::eCallableNV;
    }
    assert(false);
    return {};
}

VKPipelineProgram::VKPipelineProgram(VKDevice& device, const std::vector<std::shared_ptr<Shader>>& shaders)
    : m_device(device)
{
    for (auto& shader : shaders)
    {
        m_shaders.emplace_back(std::static_pointer_cast<SpirvShader>(shader));
    }

    for (auto& shader : m_shaders)
    {
        ShaderType shader_type = shader->GetType();
        auto& spirv = shader->GetBlob();
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        ParseShader(shader_type, spirv, bindings);

        vk::DescriptorSetLayoutCreateInfo layout_info = {};
        layout_info.bindingCount = bindings.size();
        layout_info.pBindings = bindings.data();

        m_descriptor_set_layouts.emplace_back();
        vk::UniqueDescriptorSetLayout& descriptor_set_layout = m_descriptor_set_layouts.back();
        descriptor_set_layout = device.GetDevice().createDescriptorSetLayoutUnique(layout_info);
    }

    std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;
    for (const auto& descriptor_set_layout : m_descriptor_set_layouts)
        descriptor_set_layouts.emplace_back(descriptor_set_layout.get());

    vk::PipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.setLayoutCount = descriptor_set_layouts.size();
    pipeline_layout_info.pSetLayouts = descriptor_set_layouts.data();

    //TODO
    if (m_pipeline_layout)
        m_pipeline_layout.release();
    m_pipeline_layout = device.GetDevice().createPipelineLayoutUnique(pipeline_layout_info);

    for (auto& shader : m_shaders)
    {
        ShaderType shader_type = shader->GetType();
        auto& spirv = shader->GetBlob();

        vk::ShaderModuleCreateInfo vertexShaderCreationInfo = {};
        vertexShaderCreationInfo.codeSize = sizeof(uint32_t) * spirv.size();
        vertexShaderCreationInfo.pCode = spirv.data();

        m_shader_modules[shader_type] = m_device.GetDevice().createShaderModuleUnique(vertexShaderCreationInfo);

        spirv_cross::CompilerHLSL& compiler = m_shader_ref.find(shader_type)->second.compiler;
        m_shader_ref.find(shader_type)->second.entries = compiler.get_entry_points_and_stages();
        auto& entry_points = m_shader_ref.find(shader_type)->second.entries;

        for (auto& entry_point : entry_points)
        {
            shaderStageCreateInfo.emplace_back();
            shaderStageCreateInfo.back().stage = ExecutionModel2Bit(entry_point.execution_model);

            shaderStageCreateInfo.back().module = m_shader_modules[shader_type].get();
            shaderStageCreateInfo.back().pName = entry_point.name.c_str();
            shaderStageCreateInfo.back().pSpecializationInfo = NULL;
        }
    }
}

static void print_resources(const spirv_cross::Compiler& compiler, const char* tag, const spirv_cross::SmallVector<spirv_cross::Resource>& resources)
{
    using namespace spirv_cross;
    using namespace spv;
    using namespace std;
    fprintf(stderr, "%s\n", tag);
    fprintf(stderr, "=============\n\n");
    bool print_ssbo = !strcmp(tag, "ssbos");

    for (auto& res : resources)
    {
        auto& type = compiler.get_type(res.type_id);

        if (print_ssbo && compiler.buffer_is_hlsl_counter_buffer(res.id))
            continue;

        // If we don't have a name, use the fallback for the type instead of the variable
        // for SSBOs and UBOs since those are the only meaningful names to use externally.
        // Push constant blocks are still accessed by name and not block name, even though they are technically Blocks.
        bool is_push_constant = compiler.get_storage_class(res.id) == StorageClassPushConstant;
        bool is_block = compiler.get_decoration_bitset(type.self).get(DecorationBlock) ||
            compiler.get_decoration_bitset(type.self).get(DecorationBufferBlock);
        bool is_sized_block = is_block && (compiler.get_storage_class(res.id) == StorageClassUniform ||
            compiler.get_storage_class(res.id) == StorageClassUniformConstant);
        uint32_t fallback_id = !is_push_constant && is_block ? res.base_type_id : res.id;

        uint32_t block_size = 0;
        if (is_sized_block)
            block_size = uint32_t(compiler.get_declared_struct_size(compiler.get_type(res.base_type_id)));

        Bitset mask;
        if (print_ssbo)
            mask = compiler.get_buffer_block_flags(res.id);
        else
            mask = compiler.get_decoration_bitset(res.id);

        string array;
        for (auto arr : type.array)
            array = join("[", arr ? convert_to_string(arr) : "", "]") + array;

        fprintf(stderr, " ID %03u : %s%s", res.id,
            !res.name.empty() ? res.name.c_str() : compiler.get_fallback_name(fallback_id).c_str(), array.c_str());

        if (mask.get(DecorationLocation))
            fprintf(stderr, " (Location : %u)", compiler.get_decoration(res.id, DecorationLocation));
        if (mask.get(DecorationDescriptorSet))
            fprintf(stderr, " (Set : %u)", compiler.get_decoration(res.id, DecorationDescriptorSet));
        if (mask.get(DecorationBinding))
            fprintf(stderr, " (Binding : %u)", compiler.get_decoration(res.id, DecorationBinding));
        if (mask.get(DecorationInputAttachmentIndex))
            fprintf(stderr, " (Attachment : %u)", compiler.get_decoration(res.id, DecorationInputAttachmentIndex));
        if (mask.get(DecorationNonReadable))
            fprintf(stderr, " writeonly");
        if (mask.get(DecorationNonWritable))
            fprintf(stderr, " readonly");
        if (is_sized_block)
            fprintf(stderr, " (BlockSize : %u bytes)", block_size);

        uint32_t counter_id = 0;
        if (print_ssbo && compiler.buffer_get_hlsl_counter_buffer(res.id, counter_id))
            fprintf(stderr, " (HLSL counter buffer ID: %u)", counter_id);
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "=============\n\n");
}

void VKPipelineProgram::ParseShader(ShaderType shader_type, const std::vector<uint32_t>& spirv_binary, std::vector<vk::DescriptorSetLayoutBinding>& bindings)
{
    m_shader_ref.emplace(shader_type, spirv_binary);
    spirv_cross::CompilerHLSL& compiler = m_shader_ref.find(shader_type)->second.compiler;
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    auto generate_bindings = [&](const spirv_cross::SmallVector<spirv_cross::Resource>& resources, vk::DescriptorType res_type)
    {
        for (auto& res : resources)
        {
            auto& info = m_shader_ref.find(shader_type)->second.resources[res.name];
            info.res = res;
            auto& type = compiler.get_type(res.base_type_id);

            if (type.basetype == spirv_cross::SPIRType::BaseType::Image && type.image.dim == spv::Dim::DimBuffer)
            {
                if (res_type == vk::DescriptorType::eSampledImage)
                {
                    res_type = vk::DescriptorType::eUniformTexelBuffer;
                }
                else if (res_type == vk::DescriptorType::eStorageImage)
                {
                    res_type = vk::DescriptorType::eStorageTexelBuffer;
                }
            }

            bindings.emplace_back();
            vk::DescriptorSetLayoutBinding& binding = bindings.back();
            binding.binding = compiler.get_decoration(res.id, spv::DecorationBinding);
            binding.descriptorType = res_type;
            binding.descriptorCount = 1;
            binding.stageFlags = ShaderType2Bit(shader_type);

            info.binding = binding.binding;
            info.descriptor_type = res_type;
        }
    };

    generate_bindings(resources.uniform_buffers, vk::DescriptorType::eUniformBuffer);
    generate_bindings(resources.separate_images, vk::DescriptorType::eSampledImage);
    generate_bindings(resources.separate_samplers, vk::DescriptorType::eSampler);
    generate_bindings(resources.storage_buffers, vk::DescriptorType::eStorageBuffer);
    generate_bindings(resources.storage_images, vk::DescriptorType::eStorageImage);
    generate_bindings(resources.acceleration_structures, vk::DescriptorType::eAccelerationStructureNV);

    print_resources(compiler, " stage_inputs ", resources.stage_inputs);
    print_resources(compiler, " uniform_buffers ", resources.uniform_buffers);
    print_resources(compiler, " storage_buffers ", resources.storage_buffers);
    print_resources(compiler, " storage_images ", resources.storage_images);
    print_resources(compiler, " separate_images ", resources.separate_images);
    print_resources(compiler, " separate_samplers ", resources.separate_samplers);
    print_resources(compiler, " stage_outputs ", resources.stage_outputs);
    print_resources(compiler, " acceleration_structures ", resources.acceleration_structures);
}
