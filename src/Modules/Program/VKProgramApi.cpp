#include "VKProgramApi.h"

#include <vector>
#include <list>
#include <utility>
#include <Resource/VKResource.h>
#include <View/VKView.h>
#include <Shader/SpirvCompiler.h>
#include <iostream>
#include <Utilities/VKUtility.h>

VKProgramApi::VKProgramApi(VKContext& context)
    : CommonProgramApi(context)
    , m_context(context)
    , m_view_creater(context, *this)
{
    m_depth_stencil_desc.depth_enable = true;
}

vk::ShaderStageFlagBits VKProgramApi::ShaderType2Bit(ShaderType type)
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

vk::ShaderStageFlagBits VKProgramApi::ExecutionModel2Bit(spv::ExecutionModel model)
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

void VKProgramApi::LinkProgram()
{
    ParseShaders();
    m_view_creater.OnLinkProgram();

    for (auto & shader : m_spirv)
    {
        spirv_cross::CompilerHLSL& compiler = m_shader_ref.find(shader.first)->second.compiler;
        m_shader_ref.find(shader.first)->second.entries = compiler.get_entry_points_and_stages();
        auto& entry_points = m_shader_ref.find(shader.first)->second.entries;

        for (auto& entry_point : entry_points)
        {
            shaderStageCreateInfo.emplace_back();
            shaderStageCreateInfo.back().stage = ExecutionModel2Bit(entry_point.execution_model);

            shaderStageCreateInfo.back().module = m_shaders[shader.first].get();
            shaderStageCreateInfo.back().pName = entry_point.name.c_str();
            shaderStageCreateInfo.back().pSpecializationInfo = NULL;
        }
    }

    if (m_spirv.count(ShaderType::kVertex))
    {
        CreateInputLayout(m_spirv[ShaderType::kVertex], binding_desc, attribute_desc);
    }
    if (m_spirv.count(ShaderType::kPixel))
    {
        CreateRenderPass(m_spirv[ShaderType::kPixel]);
    }
}

void VKProgramApi::CreateDRXPipeLine()
{
    std::vector<vk::RayTracingShaderGroupCreateInfoNV> groups(shaderStageCreateInfo.size());
    for (int i = 0; i < shaderStageCreateInfo.size(); ++i)
    {
        auto& group = groups[i];
        group.generalShader = VK_SHADER_UNUSED_NV;
        group.closestHitShader = VK_SHADER_UNUSED_NV;
        group.anyHitShader = VK_SHADER_UNUSED_NV;
        group.intersectionShader = VK_SHADER_UNUSED_NV;

        switch (shaderStageCreateInfo[i].stage)
        {
        case vk::ShaderStageFlagBits::eClosestHitNV:
            groups[i].type = vk::RayTracingShaderGroupTypeNV::eTrianglesHitGroup;
            groups[i].closestHitShader = i;
            break;
        case vk::ShaderStageFlagBits::eAnyHitNV:
            groups[i].type = vk::RayTracingShaderGroupTypeNV::eTrianglesHitGroup;
            groups[i].anyHitShader = i;
            break;
        case vk::ShaderStageFlagBits::eIntersectionNV:
            groups[i].type = vk::RayTracingShaderGroupTypeNV::eProceduralHitGroup;
            groups[i].intersectionShader = i;
            break;
        default:
            groups[i].type = vk::RayTracingShaderGroupTypeNV::eGeneral;
            groups[i].generalShader = i;
            break;
        }
    }

    vk::RayTracingPipelineCreateInfoNV rayPipelineInfo{};
    rayPipelineInfo.stageCount = static_cast<uint32_t>(shaderStageCreateInfo.size());
    rayPipelineInfo.pStages = shaderStageCreateInfo.data();
    rayPipelineInfo.groupCount = static_cast<uint32_t>(groups.size());
    rayPipelineInfo.pGroups = groups.data();
    rayPipelineInfo.maxRecursionDepth = 1;
    rayPipelineInfo.layout = m_pipeline_layout.get();

    //TODO
    if (graphicsPipeline)
        graphicsPipeline.release();
    graphicsPipeline = m_context.m_device->createRayTracingPipelineNVUnique({}, rayPipelineInfo);


    // Query the ray tracing properties of the current implementation, we will need them later on
    vk::PhysicalDeviceRayTracingPropertiesNV rayTracingProperties{};

    vk::PhysicalDeviceProperties2 deviceProps2{};
    deviceProps2.pNext = &rayTracingProperties;

    m_context.m_physical_device.getProperties2(&deviceProps2);

    int GroupCount = 3;

    {
        const uint32_t sbtSize = rayTracingProperties.shaderGroupHandleSize * GroupCount;

        vk::BufferCreateInfo bufferInfo = {};
        bufferInfo.size = sbtSize;
        bufferInfo.usage = vk::BufferUsageFlagBits::eRayTracingNV;
        // TODO
        if (shaderBindingTable)
            shaderBindingTable.release();
        shaderBindingTable = m_context.m_device->createBufferUnique(bufferInfo);

        vk::MemoryRequirements memRequirements;
        m_context.m_device->getBufferMemoryRequirements(shaderBindingTable.get(), &memRequirements);

        vk::MemoryAllocateInfo allocInfo = {};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = m_context.findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible);

        // TODO
        if (shaderBindingTableMemory)
            shaderBindingTableMemory.release();
        shaderBindingTableMemory = m_context.m_device->allocateMemoryUnique(allocInfo);

        m_context.m_device->bindBufferMemory(shaderBindingTable.get(), shaderBindingTableMemory.get(), 0);

        void* datav;
        m_context.m_device->mapMemory(shaderBindingTableMemory.get(), 0, bufferInfo.size, {}, &datav);
        uint8_t* data = (uint8_t*)datav;

        std::vector<uint8_t> shaderHandleStorage(sbtSize);

        m_context.m_device->getRayTracingShaderGroupHandlesNV(graphicsPipeline.get(), 0, GroupCount, sbtSize, shaderHandleStorage.data());

        auto copyShaderIdentifier = [&rayTracingProperties](uint8_t* data, const uint8_t* shaderHandleStorage, uint32_t groupIndex) {
            const uint32_t shaderGroupHandleSize = rayTracingProperties.shaderGroupHandleSize;
            memcpy(data, shaderHandleStorage + groupIndex * shaderGroupHandleSize, shaderGroupHandleSize);
            data += shaderGroupHandleSize;
            return shaderGroupHandleSize;
        };

        // Copy the shader identifiers to the shader binding table
        vk::DeviceSize offset = 0;
        for (int i = 0; i < GroupCount; ++i)
        {
            data += copyShaderIdentifier(data, shaderHandleStorage.data(), i);
        }

        m_context.m_device->unmapMemory(shaderBindingTableMemory.get());
    }
}

void VKProgramApi::CreateGrPipeLine()
{
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};

    vertexInputInfo.vertexBindingDescriptionCount = binding_desc.size();
    vertexInputInfo.pVertexBindingDescriptions = binding_desc.data();
    vertexInputInfo.vertexAttributeDescriptionCount = attribute_desc.size();
    vertexInputInfo.pVertexAttributeDescriptions = attribute_desc.data();

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    vk::PipelineViewportStateCreateInfo viewportState = {};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    vk::PipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.depthBiasEnable = m_rasterizer_desc.DepthBias != 0;
    rasterizer.depthBiasConstantFactor = m_rasterizer_desc.DepthBias;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = m_blend_desc.blend_enable;

    if (colorBlendAttachment.blendEnable)
    {
        auto convert = [](Blend type)
        {
            switch (type)
            {
            case Blend::kZero:
                return vk::BlendFactor::eZero;
            case Blend::kSrcAlpha:
                return vk::BlendFactor::eSrcAlpha;
            case Blend::kInvSrcAlpha:
                return vk::BlendFactor::eOneMinusSrcAlpha;
            }
            throw std::runtime_error("unsupported");
        };

        auto convert_op = [](BlendOp type)
        {
            switch (type)
            {
            case BlendOp::kAdd:
                return vk::BlendOp::eAdd;
            }
            throw std::runtime_error("unsupported");
        };

        colorBlendAttachment.srcColorBlendFactor = convert(m_blend_desc.blend_src);
        colorBlendAttachment.dstColorBlendFactor = convert(m_blend_desc.blend_dest);
        colorBlendAttachment.colorBlendOp = convert_op(m_blend_desc.blend_op);
        colorBlendAttachment.srcAlphaBlendFactor = convert(m_blend_desc.blend_src_alpha);
        colorBlendAttachment.dstAlphaBlendFactor = convert(m_blend_desc.blend_dest_apha);
        colorBlendAttachment.alphaBlendOp = convert_op(m_blend_desc.blend_op_alpha);
    }

    std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments(m_attachment_views.size() - 1, colorBlendAttachment);

    vk::PipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = vk::LogicOp::eAnd;
    colorBlending.attachmentCount = colorBlendAttachments.size();
    colorBlending.pAttachments = colorBlendAttachments.data();

    vk::PipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.rasterizationSamples = (vk::SampleCountFlagBits)msaa_count;
    multisampling.sampleShadingEnable = multisampling.rasterizationSamples != vk::SampleCountFlagBits::e1;

    vk::PipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.depthTestEnable = m_depth_stencil_desc.depth_enable;
    depthStencil.depthWriteEnable = m_depth_stencil_desc.depth_enable;
    depthStencil.depthCompareOp = vk::CompareOp::eLessOrEqual;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    pipelineInfo.stageCount = shaderStageCreateInfo.size();
    pipelineInfo.pStages = shaderStageCreateInfo.data();

    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;

    pipelineInfo.layout = m_pipeline_layout.get();

    pipelineInfo.renderPass = m_render_pass.get();
    pipelineInfo.subpass = 0;

    std::vector<vk::DynamicState> dynamicStateEnables = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };
    vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
    pipelineDynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();
    pipelineDynamicStateCreateInfo.dynamicStateCount = dynamicStateEnables.size();

    pipelineInfo.pDynamicState = &pipelineDynamicStateCreateInfo;

    // TODO
    if (graphicsPipeline)
        graphicsPipeline.release();
    graphicsPipeline = m_context.m_device->createGraphicsPipelineUnique({}, pipelineInfo);
}

void VKProgramApi::CreateComputePipeLine()
{
    vk::ComputePipelineCreateInfo pipeline_info = {};
    pipeline_info.stage = shaderStageCreateInfo.front();
    pipeline_info.layout = m_pipeline_layout.get();
    // TODO
    if (graphicsPipeline)
        graphicsPipeline.release();
    graphicsPipeline = m_context.m_device->createComputePipelineUnique({}, pipeline_info);
}

void VKProgramApi::CreatePipeLine()
{
    if (m_is_dxr)
        CreateDRXPipeLine();
    else if (m_is_compute)
        CreateComputePipeLine();
    else
        CreateGrPipeLine();
}

void VKProgramApi::UseProgram()
{
}

void VKProgramApi::ApplyBindings()
{
    UpdateCBuffers();

    if (m_changed_om)
    {
        if (!m_is_compute)
        {
            for (int i = 0; i < m_attachment_descriptions.size() - 1; ++i)
            {
                m_attachment_descriptions[i].loadOp = m_clear_cache.GetColorLoadOp(i);
                m_clear_cache.GetColorLoadOp(i) = vk::AttachmentLoadOp::eLoad;
            }
            if (m_attachment_views.back())
                m_attachment_descriptions.back().loadOp = m_clear_cache.GetDepthLoadOp();
            m_clear_cache.GetDepthLoadOp() = vk::AttachmentLoadOp::eLoad;
            vk::SubpassDescription subPass = {};
            subPass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
            subPass.colorAttachmentCount = m_attachment_references.size() - 1;
            subPass.pColorAttachments = m_attachment_references.data();
            subPass.pDepthStencilAttachment = &m_attachment_references.back();
            if (!m_attachment_views.back())
                subPass.pDepthStencilAttachment = nullptr;

            vk::RenderPassCreateInfo renderPassInfo = {};
            renderPassInfo.attachmentCount = m_attachment_descriptions.size();
            if (!m_attachment_views.back())
                --renderPassInfo.attachmentCount;
            renderPassInfo.pAttachments = m_attachment_descriptions.data();
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subPass;

            vk::AccessFlags AccessMask =
                vk::AccessFlagBits::eIndirectCommandRead |
                vk::AccessFlagBits::eIndexRead |
                vk::AccessFlagBits::eVertexAttributeRead |
                vk::AccessFlagBits::eUniformRead |
                vk::AccessFlagBits::eInputAttachmentRead |
                vk::AccessFlagBits::eShaderRead |
                vk::AccessFlagBits::eShaderWrite |
                vk::AccessFlagBits::eColorAttachmentRead |
                vk::AccessFlagBits::eColorAttachmentWrite |
                vk::AccessFlagBits::eDepthStencilAttachmentRead |
                vk::AccessFlagBits::eDepthStencilAttachmentWrite |
                vk::AccessFlagBits::eTransferRead |
                vk::AccessFlagBits::eTransferWrite |
                vk::AccessFlagBits::eHostRead |
                vk::AccessFlagBits::eHostWrite |
                vk::AccessFlagBits::eMemoryRead |
                vk::AccessFlagBits::eMemoryWrite;

            vk::SubpassDependency self_dependencie = {};
            self_dependencie.srcStageMask = vk::PipelineStageFlagBits::eAllCommands;
            self_dependencie.dstStageMask = vk::PipelineStageFlagBits::eAllCommands;
            self_dependencie.srcAccessMask = AccessMask;
            self_dependencie.dstAccessMask = AccessMask;
            self_dependencie.dependencyFlags = vk::DependencyFlagBits::eByRegion;

            renderPassInfo.dependencyCount = 1;
            renderPassInfo.pDependencies = &self_dependencie;

            // TODO
            if (m_render_pass)
                m_render_pass.release();
            m_render_pass = m_context.m_device->createRenderPassUnique(renderPassInfo);

            vk::FramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.renderPass = m_render_pass.get();
            framebufferInfo.attachmentCount = m_attachment_views.size();
            if (!m_attachment_views.back())
                --framebufferInfo.attachmentCount;
            framebufferInfo.pAttachments = m_attachment_views.data();
            framebufferInfo.width = m_rtv_size[0].first.width;
            framebufferInfo.height = m_rtv_size[0].first.height;
            framebufferInfo.layers = m_rtv_size[0].second;

            // TODO
            if (m_framebuffer)
                m_framebuffer.release();
            m_framebuffer = m_context.m_device->createFramebufferUnique(framebufferInfo);
        }
        m_changed_om = false;
        CreatePipeLine();
    }

    if (m_is_dxr)
        m_context.m_cmd_bufs[m_context.GetFrameIndex()]->bindPipeline(vk::PipelineBindPoint::eRayTracingNV, graphicsPipeline.get());
    else if (m_is_compute)
        m_context.m_cmd_bufs[m_context.GetFrameIndex()]->bindPipeline(vk::PipelineBindPoint::eCompute, graphicsPipeline.get());
    else
        m_context.m_cmd_bufs[m_context.GetFrameIndex()]->bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline.get());

    std::map<BindKey, View::Ptr> descriptor_cache;
    for (auto& x : m_bound_resources)
    {
        const BindKey& bind_key = x.first;
        switch (bind_key.res_type)
        {
        case ResourceType::kCbv:
        case ResourceType::kSrv:
        case ResourceType::kUav:
        case ResourceType::kSampler:
            descriptor_cache[bind_key] = x.second.view;
            break;
        }
    }

    auto it = m_heap_cache.find(descriptor_cache);
    if (it == m_heap_cache.end())
    {
        std::vector<vk::UniqueDescriptorSet> descriptor_sets;
        for (size_t i = 0; i < m_descriptor_set_layouts.size(); ++i)
        {
            descriptor_sets.emplace_back(m_context.GetDescriptorPool().AllocateDescriptorSet(m_descriptor_set_layouts[i].get(), m_descriptor_count_by_set[i]));
        }

        std::vector<vk::WriteDescriptorSet> descriptorWrites;
        std::list<vk::DescriptorImageInfo> list_image_info;
        std::list<vk::DescriptorBufferInfo> list_buffer_info;
        std::list<vk::WriteDescriptorSetAccelerationStructureNV> list_as;

        for (auto& x : m_bound_resources)
        {
            bool is_rtv_dsv = false;
            switch (x.first.res_type)
            {
            case ResourceType::kRtv:
            case ResourceType::kDsv:
                is_rtv_dsv = true;
                break;
            }

            if (is_rtv_dsv || !x.second.res)
                continue;

            auto& view = static_cast<VKView&>(*x.second.view);
            ShaderType shader_type = x.first.shader_type;
            ShaderRef& shader_ref = m_shader_ref.find(shader_type)->second;
            std::string name = GetBindingName(x.first);
            if (name == "$Globals")
                name = "_Global";

            if (!shader_ref.resources.count(name))
                name = "type_" + name;

            if (!shader_ref.resources.count(name))
                throw std::runtime_error("failed to find resource reflection");
            auto ref_res = shader_ref.resources[name];
            VKResource& res = static_cast<VKResource&>(*x.second.res);

            vk::WriteDescriptorSet descriptorWrite = {};
            descriptorWrite.dstSet = descriptor_sets[GetSetNumByShaderType(shader_type)].get();
            descriptorWrite.dstBinding = ref_res.binding;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = ref_res.descriptor_type;
            descriptorWrite.descriptorCount = 1;

            switch (ref_res.descriptor_type)
            {
            case vk::DescriptorType::eSampler:
            {
                list_image_info.emplace_back();
                vk::DescriptorImageInfo& image_info = list_image_info.back();
                image_info.sampler = res.sampler.res.get();
                descriptorWrite.pImageInfo = &image_info;
                break;
            }
            case vk::DescriptorType::eSampledImage:
            {
                list_image_info.emplace_back();
                vk::DescriptorImageInfo& image_info = list_image_info.back();
                image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                image_info.imageView = view.srv.get();
                descriptorWrite.pImageInfo = &image_info;
                break;
            }
            case vk::DescriptorType::eStorageImage:
            {
                list_image_info.emplace_back();
                vk::DescriptorImageInfo& image_info = list_image_info.back();
                image_info.imageLayout = vk::ImageLayout::eGeneral;
                image_info.imageView = view.srv.get();
                descriptorWrite.pImageInfo = &image_info;
                break;
            }
            case vk::DescriptorType::eUniformTexelBuffer:
            case vk::DescriptorType::eStorageTexelBuffer:
            case vk::DescriptorType::eUniformBuffer:
            case vk::DescriptorType::eStorageBuffer:
            {
                list_buffer_info.emplace_back();
                vk::DescriptorBufferInfo& buffer_info = list_buffer_info.back();
                buffer_info.buffer = res.buffer.res.get();
                buffer_info.offset = 0;
                buffer_info.range = VK_WHOLE_SIZE;
                descriptorWrite.pBufferInfo = &buffer_info;
                break;
            }
            case vk::DescriptorType::eAccelerationStructureNV:
            {
                list_as.emplace_back();
                vk::WriteDescriptorSetAccelerationStructureNV& descriptorAccelerationStructureInfo = list_as.back();
                descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
                descriptorAccelerationStructureInfo.pAccelerationStructures = &res.top_as.accelerationStructure.get();

                descriptorWrite.pNext = &descriptorAccelerationStructureInfo;
                break;
            }
            case vk::DescriptorType::eUniformBufferDynamic:
            case vk::DescriptorType::eStorageBufferDynamic:
            default:
                ASSERT(false);
                break;
            }

            if (descriptorWrite.pImageInfo || descriptorWrite.pBufferInfo || descriptorWrite.pNext)
                descriptorWrites.push_back(descriptorWrite);
        }

        if (!descriptorWrites.empty())
            m_context.m_device->updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
        it = m_heap_cache.emplace(descriptor_cache, std::move(descriptor_sets)).first;
    }

    std::vector<vk::DescriptorSet> descriptor_sets;
    for (const auto& descriptor_set : it->second)
    {
        descriptor_sets.emplace_back(descriptor_set.get());
    }

    if (m_is_dxr)
        m_context.m_cmd_bufs[m_context.GetFrameIndex()]->bindDescriptorSets(vk::PipelineBindPoint::eRayTracingNV, m_pipeline_layout.get(), 0,
            descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);
    else if (m_is_compute)
        m_context.m_cmd_bufs[m_context.GetFrameIndex()]->bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_pipeline_layout.get(), 0,
            descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);
    else
        m_context.m_cmd_bufs[m_context.GetFrameIndex()]->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline_layout.get(), 0,
            descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);
}

View::Ptr VKProgramApi::CreateView(const BindKey& bind_key, const ViewDesc& view_desc, const Resource::Ptr& res)
{
    return m_view_creater.GetView(m_program_id, bind_key.shader_type, bind_key.res_type, bind_key.slot, view_desc, GetBindingName(bind_key), res);
}

void VKProgramApi::RenderPassBegin()
{
    vk::RenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.renderPass = m_render_pass.get();
    renderPassInfo.framebuffer = m_framebuffer.get();

    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_rtv_size[0].first;

    std::vector<vk::ClearValue> clearValues(m_attachment_views.size() - 1);
    for (int i = 0; i < m_attachment_views.size() - 1; ++i)
    {
        clearValues[i].color = m_clear_cache.GetColor(i);
    }
    if (m_attachment_views.back())
    {
        clearValues.emplace_back();
        clearValues.back().depthStencil = m_clear_cache.GetDepth();
    }

    renderPassInfo.clearValueCount = clearValues.size();
    renderPassInfo.pClearValues = clearValues.data();

    m_context.m_cmd_bufs[m_context.GetFrameIndex()]->beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
}

void VKProgramApi::CompileShader(const ShaderBase& shader)
{
    if (shader.type == ShaderType::kCompute)
        m_is_compute = true;

    if (shader.type == ShaderType::kLibrary)
    {
        m_is_compute = true;
        m_is_dxr = true;
    }

    SpirvOption option;
    option.auto_map_bindings = true;
    option.hlsl_iomap = true;
    option.invert_y = true;
    if (shader.type == ShaderType::kLibrary)
        option.use_dxc = true;
    if (m_shader_types.count(ShaderType::kGeometry) && shader.type == ShaderType::kVertex)
        option.invert_y = false;
    option.resource_set_binding = GetSetNumByShaderType(shader.type);
    auto spirv = SpirvCompile(shader, option);
    m_spirv[shader.type] = spirv;

    vk::ShaderModuleCreateInfo vertexShaderCreationInfo = {};
    vertexShaderCreationInfo.codeSize = sizeof(uint32_t) * spirv.size();
    vertexShaderCreationInfo.pCode = spirv.data();

    m_shaders[shader.type] = m_context.m_device->createShaderModuleUnique(vertexShaderCreationInfo);;
    m_shaders_info2[shader.type] = &shader;
}

static void print_resources(const spirv_cross::Compiler &compiler, const char *tag, const spirv_cross::SmallVector<spirv_cross::Resource> &resources)
{
    using namespace spirv_cross;
    using namespace spv;
    using namespace std;
    fprintf(stderr, "%s\n", tag);
    fprintf(stderr, "=============\n\n");
    bool print_ssbo = !strcmp(tag, "ssbos");

    for (auto &res : resources)
    {
        auto &type = compiler.get_type(res.type_id);

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

void VKProgramApi::ParseShader(ShaderType shader_type, const std::vector<uint32_t>& spirv_binary, std::vector<vk::DescriptorSetLayoutBinding>& bindings)
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
            auto &type = compiler.get_type(res.base_type_id);

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

size_t VKProgramApi::GetSetNumByShaderType(ShaderType type)
{
    if (m_shader_type2set.count(type))
        return m_shader_type2set[type];
    size_t num = m_shader_type2set.size();
    m_shader_type2set[type] = num;
    return num;
}

void VKProgramApi::ParseShaders()
{
    for (auto & spirv_it : m_spirv)
    {
        auto& spirv = spirv_it.second;
        std::cout << std::endl << m_shaders_info2[spirv_it.first]->shader_path << std::endl;

        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        ParseShader(spirv_it.first, spirv, bindings);

        vk::DescriptorSetLayoutCreateInfo layout_info = {};
        layout_info.bindingCount = bindings.size();
        layout_info.pBindings = bindings.data();

        int set_num = GetSetNumByShaderType(spirv_it.first);
        if (m_descriptor_set_layouts.size() <= set_num)
        {
            m_descriptor_set_layouts.resize(set_num + 1);
            m_descriptor_count_by_set.resize(set_num + 1);
        }

        vk::UniqueDescriptorSetLayout& descriptor_set_layout = m_descriptor_set_layouts[set_num];
        descriptor_set_layout = m_context.m_device->createDescriptorSetLayoutUnique(layout_info);

        for (auto & binding : bindings)
        {
            m_descriptor_count_by_set[set_num][binding.descriptorType] += binding.descriptorCount;
        }
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
    m_pipeline_layout = m_context.m_device->createPipelineLayoutUnique(pipeline_layout_info);
}

void VKProgramApi::OnPresent()
{
    m_cbv_offset.get().clear();
    m_changed_om = true;
}

ShaderBlob VKProgramApi::GetBlobByType(ShaderType type) const
{
    auto it = m_spirv.find(type);
    if (it == m_spirv.end())
        return {};

    return { (uint8_t*)it->second.data(), sizeof(uint32_t) * it->second.size() };
    return ShaderBlob();
}

void VKProgramApi::OnAttachSRV(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const Resource::Ptr& ires)
{
    if (!ires)
        return;

    VKResource& res = static_cast<VKResource&>(*ires);

    if (res.image.res)
        m_context.TransitionImageLayout(res.image, vk::ImageLayout::eShaderReadOnlyOptimal, view_desc);
}

void VKProgramApi::OnAttachUAV(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const Resource::Ptr& ires)
{
    if (!ires)
        return;

    VKResource& res = static_cast<VKResource&>(*ires);

    if (res.image.res)
        m_context.TransitionImageLayout(res.image, vk::ImageLayout::eGeneral, view_desc);
}

void VKProgramApi::OnAttachCBV(ShaderType type, uint32_t slot, const Resource::Ptr & ires)
{
}

void VKProgramApi::OnAttachSampler(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires)
{
}

VKView* ConvertView(const View::Ptr& view)
{
    if (!view)
        return nullptr;
    return &static_cast<VKView&>(*view);
}

void VKProgramApi::OnAttachRTV(uint32_t slot, const ViewDesc& view_desc, const Resource::Ptr & ires)
{
    m_changed_om = true;
    if (m_context.m_is_open_render_pass)
    {
        m_context.m_cmd_bufs[m_context.GetFrameIndex()]->endRenderPass();
        m_context.m_is_open_render_pass = false;
    }

    if (!ires)
        return;

    VKResource& res = static_cast<VKResource&>(*ires);

    m_context.TransitionImageLayout(res.image, vk::ImageLayout::eColorAttachmentOptimal, view_desc);

    auto view = ConvertView(FindView(ShaderType::kPixel, ResourceType::kRtv, slot));
    m_attachment_views[slot] = view->om.get();
    m_rtv_size[slot] = { { res.image.size.width >> view_desc.level, res.image.size.height >> view_desc.level}, res.image.array_layers };
 
    vk::AttachmentDescription& colorAttachment = m_attachment_descriptions[slot];
    colorAttachment.format = res.image.format;
    colorAttachment.samples = (vk::SampleCountFlagBits)res.image.msaa_count;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

    msaa_count = res.image.msaa_count;

    vk::AttachmentReference& colorAttachmentRef = m_attachment_references[slot];
    colorAttachmentRef.attachment = slot;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal; 
}

void VKProgramApi::OnAttachDSV(const ViewDesc& view_desc, const Resource::Ptr & ires)
{
    m_changed_om = true;
    if (m_context.m_is_open_render_pass)
    {
        m_context.m_cmd_bufs[m_context.GetFrameIndex()]->endRenderPass();
        m_context.m_is_open_render_pass = false;
    }

    if (!ires)
        return;

    VKResource& res = static_cast<VKResource&>(*ires);

    m_context.TransitionImageLayout(res.image, vk::ImageLayout::eDepthStencilAttachmentOptimal, view_desc);

    auto view = ConvertView(FindView(ShaderType::kPixel, ResourceType::kDsv, 0));

    m_attachment_views.back() = view->om.get();
    m_rtv_size.back() = { { res.image.size.width >> view_desc.level, (res.image.size.height >> view_desc.level)}, res.image.array_layers };

    auto& m_depth_attachment = m_attachment_descriptions.back();
    m_depth_attachment.format = res.image.format;
    m_depth_attachment.samples = (vk::SampleCountFlagBits)res.image.msaa_count;
    m_depth_attachment.loadOp = vk::AttachmentLoadOp::eClear;
    m_depth_attachment.storeOp = vk::AttachmentStoreOp::eStore;
    m_depth_attachment.stencilLoadOp = vk::AttachmentLoadOp::eClear;
    m_depth_attachment.stencilStoreOp = vk::AttachmentStoreOp::eStore;
    m_depth_attachment.initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    m_depth_attachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    msaa_count = res.image.msaa_count;

    vk::AttachmentReference& depthAttachmentReference = m_attachment_references.back();
    depthAttachmentReference.attachment = m_attachment_descriptions.size() - 1;
    depthAttachmentReference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
}

void VKProgramApi::ClearRenderTarget(uint32_t slot, const std::array<float, 4>& color)
{
    auto& clear_color = m_clear_cache.GetColor(slot);
    clear_color.float32[0] = color[0];
    clear_color.float32[1] = color[1];
    clear_color.float32[2] = color[2];
    clear_color.float32[3] = color[3];
    m_clear_cache.GetColorLoadOp(slot) = vk::AttachmentLoadOp::eClear;
}

void VKProgramApi::ClearDepthStencil(uint32_t ClearFlags, float Depth, uint8_t Stencil)
{
    m_clear_cache.GetDepth() = { Depth, Stencil };
    m_clear_cache.GetDepthLoadOp() = vk::AttachmentLoadOp::eClear;
}

void VKProgramApi::SetRasterizeState(const RasterizerDesc& desc)
{
    m_rasterizer_desc = desc;
    m_changed_om = true;
}

void VKProgramApi::SetBlendState(const BlendDesc& desc)
{
    m_blend_desc = desc;
    m_changed_om = true;
}

void VKProgramApi::SetDepthStencilState(const DepthStencilDesc& desc)
{
    m_depth_stencil_desc = desc;
    m_changed_om = true;
}

void VKProgramApi::CreateInputLayout(
    const std::vector<uint32_t>& spirv_binary,
    std::vector<vk::VertexInputBindingDescription>& binding_desc,
    std::vector<vk::VertexInputAttributeDescription>& attribute_desc)
{
    spirv_cross::CompilerHLSL compiler(std::move(spirv_binary));
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();
    for (auto& resource : resources.stage_inputs)
    {
        auto &type = compiler.get_type(resource.base_type_id);
        unsigned location = compiler.get_decoration(resource.id, spv::DecorationLocation);

        binding_desc.emplace_back();
        auto& binding = binding_desc.back();
        attribute_desc.emplace_back();
        auto& attribute = attribute_desc.back();

        attribute.binding = location;
        attribute.location = location;
        binding.binding = location;
        binding.inputRate = vk::VertexInputRate::eVertex;
        binding.stride = type.vecsize * type.width / 8;

        if (type.basetype == spirv_cross::SPIRType::Float)
        {
            if (type.vecsize == 1)
                attribute.format = vk::Format::eR32Sfloat;
            else if (type.vecsize == 2)
                attribute.format = vk::Format::eR32G32Sfloat;
            else if (type.vecsize == 3)
                attribute.format = vk::Format::eR32G32B32Sfloat;
            else if (type.vecsize == 4)
                attribute.format = vk::Format::eR32G32B32A32Sfloat;
        }
        else if (type.basetype == spirv_cross::SPIRType::UInt)
        {
            if (type.vecsize == 1)
                attribute.format = vk::Format::eR32Uint;
            else if (type.vecsize == 2)
                attribute.format = vk::Format::eR32G32Uint;
            else if (type.vecsize == 3)
                attribute.format = vk::Format::eR32G32B32Uint;
            else if (type.vecsize == 4)
                attribute.format = vk::Format::eR32G32B32A32Uint;
        }
        else if (type.basetype == spirv_cross::SPIRType::Int)
        {
            if (type.vecsize == 1)
                attribute.format = vk::Format::eR32Sint;
            else if (type.vecsize == 2)
                attribute.format = vk::Format::eR32G32Sint;
            else if (type.vecsize == 3)
                attribute.format = vk::Format::eR32G32B32Sint;
            else if (type.vecsize == 4)
                attribute.format = vk::Format::eR32G32B32A32Sint;
        }
    }
}

void VKProgramApi::CreateRenderPass(const std::vector<uint32_t>& spirv_binary)
{
    spirv_cross::CompilerHLSL compiler(std::move(spirv_binary));
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();
    for (auto& resource : resources.stage_outputs)
    {
        auto &type = compiler.get_type(resource.base_type_id);
        size_t location = compiler.get_decoration(resource.id, spv::DecorationLocation);
        m_num_rtv = std::max(m_num_rtv, location + 1);
    }

    m_attachment_descriptions.resize(m_num_rtv + 1);
    m_attachment_references.resize(m_num_rtv + 1);
    m_attachment_views.resize(m_num_rtv + 1);
    m_rtv_size.resize(m_num_rtv + 1);
}
