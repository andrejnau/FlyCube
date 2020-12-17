#include "Pipeline/VKRayTracingPipeline.h"
#include "Pipeline/VKGraphicsPipeline.h"
#include <Device/VKDevice.h>
#include <Adapter/VKAdapter.h>
#include <Program/VKProgram.h>
#include <Shader/SpirvShader.h>
#include <map>

VKRayTracingPipeline::VKRayTracingPipeline(VKDevice& device, const ComputePipelineDesc& desc)
    : m_device(device)
    , m_desc(desc)
{
    decltype(auto) vk_program = desc.program->As<VKProgram>();
    auto shaders = vk_program.GetShaders();
    for (auto& shader : shaders)
    {
        ShaderType shader_type = shader->GetType();
        auto blob = shader->GetBlob();
        vk::ShaderModuleCreateInfo shader_module_info = {};
        shader_module_info.codeSize = sizeof(uint32_t) * blob.size();
        shader_module_info.pCode = blob.data();

        m_shader_modules[shader_type] = m_device.GetDevice().createShaderModuleUnique(shader_module_info);

        spirv_cross::CompilerHLSL compiler(blob);
        m_entries[shader_type] = compiler.get_entry_points_and_stages();

        for (auto& entry_point : m_entries[shader_type])
        {
            m_shader_stage_create_info.emplace_back();
            m_shader_stage_create_info.back().stage = ExecutionModel2Bit(entry_point.execution_model);
            m_shader_stage_create_info.back().module = m_shader_modules[shader_type].get();
            m_shader_stage_create_info.back().pName = entry_point.name.c_str();
            m_shader_stage_create_info.back().pSpecializationInfo = NULL;
        }
    }

    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> groups(m_shader_stage_create_info.size());
    for (int i = 0; i < m_shader_stage_create_info.size(); ++i)
    {
        auto& group = groups[i];
        group.generalShader = VK_SHADER_UNUSED_NV;
        group.closestHitShader = VK_SHADER_UNUSED_NV;
        group.anyHitShader = VK_SHADER_UNUSED_NV;
        group.intersectionShader = VK_SHADER_UNUSED_NV;

        switch (m_shader_stage_create_info[i].stage)
        {
        case vk::ShaderStageFlagBits::eClosestHitKHR:
            groups[i].type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
            groups[i].closestHitShader = i;
            break;
        case vk::ShaderStageFlagBits::eAnyHitKHR:
            groups[i].type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
            groups[i].anyHitShader = i;
            break;
        case vk::ShaderStageFlagBits::eIntersectionKHR:
            groups[i].type = vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup;
            groups[i].intersectionShader = i;
            break;
        default:
            groups[i].type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
            groups[i].generalShader = i;
            break;
        }
    }

    vk::RayTracingPipelineCreateInfoKHR ray_pipeline_info{};
    ray_pipeline_info.stageCount = static_cast<uint32_t>(m_shader_stage_create_info.size());
    ray_pipeline_info.pStages = m_shader_stage_create_info.data();
    ray_pipeline_info.groupCount = static_cast<uint32_t>(groups.size());
    ray_pipeline_info.pGroups = groups.data();
    ray_pipeline_info.maxPipelineRayRecursionDepth = 1;
    ray_pipeline_info.layout = vk_program.GetPipelineLayout();

    m_pipeline = m_device.GetDevice().createRayTracingPipelineKHRUnique({}, {}, ray_pipeline_info);

    // Query the ray tracing properties of the current implementation, we will need them later on
    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_properties = {};

    vk::PhysicalDeviceProperties2 device_props2 = {};
    device_props2.pNext = &ray_tracing_properties;

    m_device.GetAdapter().GetPhysicalDevice().getProperties2(&device_props2);

    constexpr uint32_t group_count = 3;
    vk::BufferCreateInfo buffer_info = {};
    buffer_info.size = ray_tracing_properties.shaderGroupBaseAlignment * group_count;
    buffer_info.usage = vk::BufferUsageFlagBits::eShaderDeviceAddressKHR | vk::BufferUsageFlagBits::eShaderBindingTableKHR;
    m_shader_binding_table = m_device.GetDevice().createBufferUnique(buffer_info);

    vk::MemoryRequirements mem_requirements;
    m_device.GetDevice().getBufferMemoryRequirements(m_shader_binding_table.get(), &mem_requirements);

    vk::MemoryAllocateFlagsInfo alloc_flag_info = {};
    alloc_flag_info.flags = vk::MemoryAllocateFlagBits::eDeviceAddress;

    vk::MemoryAllocateInfo allocInfo = {};
    allocInfo.pNext = &alloc_flag_info;
    allocInfo.allocationSize = mem_requirements.size;
    allocInfo.memoryTypeIndex = m_device.FindMemoryType(mem_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible);

    m_shader_binding_table_memory = m_device.GetDevice().allocateMemoryUnique(allocInfo);

    m_device.GetDevice().bindBufferMemory(m_shader_binding_table.get(), m_shader_binding_table_memory.get(), 0);

    void* datav;
    m_device.GetDevice().mapMemory(m_shader_binding_table_memory.get(), 0, buffer_info.size, {}, &datav);
    uint8_t* data = (uint8_t*)datav;

    std::vector<uint8_t> shader_handle_storage(buffer_info.size);

    m_device.GetDevice().getRayTracingShaderGroupHandlesKHR(m_pipeline.get(), 0, group_count, buffer_info.size, shader_handle_storage.data());

    auto copy_shader_identifier = [&ray_tracing_properties](uint8_t* data, const uint8_t* shader_handle_storage, uint32_t group_index)
    {
        memcpy(data, shader_handle_storage + group_index * ray_tracing_properties.shaderGroupHandleSize, ray_tracing_properties.shaderGroupHandleSize);
        return ray_tracing_properties.shaderGroupBaseAlignment;
    };

    for (int i = 0; i < group_count; ++i)
    {
        data += copy_shader_identifier(data, shader_handle_storage.data(), i);
    }

    m_device.GetDevice().unmapMemory(m_shader_binding_table_memory.get());
}

PipelineType VKRayTracingPipeline::GetPipelineType() const
{
    return PipelineType::kRayTracing;
}

vk::Pipeline VKRayTracingPipeline::GetPipeline() const
{
    return m_pipeline.get();
}

vk::Buffer VKRayTracingPipeline::GetShaderBindingTable() const
{
    return m_shader_binding_table.get();
}
