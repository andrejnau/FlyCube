#include "CommandList/VKCommandList.h"
#include <Device/VKDevice.h>
#include <Adapter/VKAdapter.h>
#include <Resource/VKResource.h>
#include <View/VKView.h>
#include <Pipeline/VKGraphicsPipeline.h>
#include <Pipeline/VKComputePipeline.h>
#include <Pipeline/VKRayTracingPipeline.h>
#include <Framebuffer/VKFramebuffer.h>
#include <BindingSet/VKBindingSet.h>
#include <Utilities/VKUtility.h>

VKCommandList::VKCommandList(VKDevice& device, CommandListType type)
    : m_device(device)
{
    vk::CommandBufferAllocateInfo cmd_buf_alloc_info = {};
    cmd_buf_alloc_info.commandPool = device.GetCmdPool(type);
    cmd_buf_alloc_info.commandBufferCount = 1;
    cmd_buf_alloc_info.level = vk::CommandBufferLevel::ePrimary;
    std::vector<vk::UniqueCommandBuffer> cmd_bufs = device.GetDevice().allocateCommandBuffersUnique(cmd_buf_alloc_info);
    m_command_list = std::move(cmd_bufs.front());
    vk::CommandBufferBeginInfo begin_info = {};
    begin_info.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;
    m_command_list->begin(begin_info);
}

void VKCommandList::Reset()
{
    Close();
    m_state.reset();
    m_binding_set.reset();
    vk::CommandBufferBeginInfo begin_info = {};
    m_command_list->begin(begin_info);
    m_closed = false;
}

void VKCommandList::Close()
{
    if (!m_closed)
    {
        m_command_list->end();
        m_closed = true;
    }
}

void VKCommandList::BindPipeline(const std::shared_ptr<Pipeline>& state)
{
    if (state == m_state)
        return;
    auto type = state->GetPipelineType();
    if (type == PipelineType::kGraphics)
    {
        decltype(auto) vk_state = state->As<VKGraphicsPipeline>();
        m_command_list->bindPipeline(vk::PipelineBindPoint::eGraphics, vk_state.GetPipeline());
    }
    else if (type == PipelineType::kCompute)
    {
        decltype(auto) vk_state = state->As<VKComputePipeline>();
        m_command_list->bindPipeline(vk::PipelineBindPoint::eCompute, vk_state.GetPipeline());
    }
    else if (type == PipelineType::kRayTracing)
    {
        decltype(auto) vk_state = state->As<VKRayTracingPipeline>();
        m_command_list->bindPipeline(vk::PipelineBindPoint::eRayTracingNV, vk_state.GetPipeline());
    }
    m_state = state;
}

void VKCommandList::BindBindingSet(const std::shared_ptr<BindingSet>& binding_set)
{
    if (binding_set == m_binding_set)
        return;
    m_binding_set = binding_set;
    decltype(auto) vk_binding_set = binding_set->As<VKBindingSet>();
    decltype(auto) descriptor_sets = vk_binding_set.GetDescriptorSets();
    if (descriptor_sets.empty())
        return;
    auto type = m_state->GetPipelineType();
    if (type == PipelineType::kGraphics)
    {
        m_command_list->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, vk_binding_set.GetPipelineLayout(), 0, descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);
    }
    else if (type == PipelineType::kCompute)
    {
        m_command_list->bindDescriptorSets(vk::PipelineBindPoint::eCompute, vk_binding_set.GetPipelineLayout(), 0, descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);
    }
    else if (type == PipelineType::kRayTracing)
    {
        m_command_list->bindDescriptorSets(vk::PipelineBindPoint::eRayTracingNV, vk_binding_set.GetPipelineLayout(), 0, descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);
    }
}

void VKCommandList::BeginRenderPass(const std::shared_ptr<RenderPass>& render_pass, const std::shared_ptr<Framebuffer>& framebuffer, const ClearDesc& clear_desc)
{
    decltype(auto) vk_framebuffer = framebuffer->As<VKFramebuffer>();
    decltype(auto) vk_render_pass = render_pass->As<VKRenderPass>();
    vk::RenderPassBeginInfo render_pass_info = {};
    render_pass_info.renderPass = vk_render_pass.GetRenderPass();
    render_pass_info.framebuffer = vk_framebuffer.GetFramebuffer();
    render_pass_info.renderArea.extent = vk_framebuffer.GetExtent();
    std::vector<vk::ClearValue> clear_values;
    for (size_t i = 0; i < clear_desc.colors.size(); ++i)
    {
        auto& clear_value = clear_values.emplace_back();
        clear_value.color.float32[0] = clear_desc.colors[i].r;
        clear_value.color.float32[1] = clear_desc.colors[i].g;
        clear_value.color.float32[2] = clear_desc.colors[i].b;
        clear_value.color.float32[3] = clear_desc.colors[i].a;
    }
    clear_values.resize(vk_render_pass.GetDesc().colors.size());
    if (vk_render_pass.GetDesc().depth_stencil.format != gli::FORMAT_UNDEFINED)
    {
        vk::ClearValue clear_value = {};
        clear_value.depthStencil.depth = clear_desc.depth;
        clear_values.emplace_back(clear_value);
    }
    render_pass_info.clearValueCount = clear_values.size();
    render_pass_info.pClearValues = clear_values.data();
    m_command_list->beginRenderPass(render_pass_info, vk::SubpassContents::eInline);
}

void VKCommandList::EndRenderPass()
{
    m_command_list->endRenderPass();
}

void VKCommandList::BeginEvent(const std::string& name)
{
    vk::DebugUtilsLabelEXT label = {};
    label.pLabelName = name.c_str();
    m_command_list->beginDebugUtilsLabelEXT(&label);
}

void VKCommandList::EndEvent()
{
    m_command_list->endDebugUtilsLabelEXT();
}

void VKCommandList::ClearColor(const std::shared_ptr<View>& view, const glm::vec4& color)
{
    if (!view)
        return;
    decltype(auto) vk_view = view->As<VKView>();

    std::shared_ptr<Resource> resource = vk_view.GetResource();
    if (!resource)
        return;
    decltype(auto) vk_resource = resource->As<VKResource>();

    vk::ClearColorValue clear_color = {};
    clear_color.float32[0] = color[0];
    clear_color.float32[1] = color[1];
    clear_color.float32[2] = color[2];
    clear_color.float32[3] = color[3];

    const vk::ImageSubresourceRange& image_subresource_range = vk_view.GeViewInfo().subresourceRange;
    m_command_list->clearColorImage(vk_resource.image.res, vk::ImageLayout::eTransferDstOptimal, clear_color, image_subresource_range);
}

void VKCommandList::ClearDepth(const std::shared_ptr<View>& view, float depth)
{
    if (!view)
        return;
    decltype(auto) vk_view = view->As<VKView>();

    std::shared_ptr<Resource> resource = vk_view.GetResource();
    if (!resource)
        return;
    decltype(auto) vk_resource = resource->As<VKResource>();

    vk::ClearDepthStencilValue clear_color = {};
    clear_color.depth = depth;
    const vk::ImageSubresourceRange& ImageSubresourceRange = vk_view.GeViewInfo().subresourceRange;
    m_command_list->clearDepthStencilImage(vk_resource.image.res, vk::ImageLayout::eTransferDstOptimal, clear_color, ImageSubresourceRange);
}

void VKCommandList::DrawIndexed(uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location)
{
    m_command_list->drawIndexed(index_count, 1, start_index_location, base_vertex_location, 0);
}

void VKCommandList::Dispatch(uint32_t thread_group_count_x, uint32_t thread_group_count_y, uint32_t thread_group_count_z)
{
    m_command_list->dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z);
}

void VKCommandList::DispatchRays(uint32_t width, uint32_t height, uint32_t depth)
{
    decltype(auto) vk_state = m_state->As<VKRayTracingPipeline>();
    auto shader_binding_table = vk_state.GetShaderBindingTable();

    vk::PhysicalDeviceRayTracingPropertiesNV ray_tracing_properties{};

    vk::PhysicalDeviceProperties2 device_props2{};
    device_props2.pNext = &ray_tracing_properties;
    m_device.GetAdapter().GetPhysicalDevice().getProperties2(&device_props2);

    vk::DeviceSize binding_offset_ray_gen_shader = ray_tracing_properties.shaderGroupBaseAlignment * 0;
    vk::DeviceSize binding_offset_miss_shader = ray_tracing_properties.shaderGroupBaseAlignment * 1;
    vk::DeviceSize binding_offset_hit_shader = ray_tracing_properties.shaderGroupBaseAlignment * 2;
    vk::DeviceSize binding_stride = ray_tracing_properties.shaderGroupHandleSize;

    m_command_list->traceRaysNV(
        shader_binding_table, binding_offset_ray_gen_shader,
        shader_binding_table, binding_offset_miss_shader, binding_stride,
        shader_binding_table, binding_offset_hit_shader, binding_stride,
        {}, 0, 0,
        width, height, depth
    );
}

vk::ImageLayout ConvertSate(ResourceState state)
{
    switch (state)
    {
    case ResourceState::kUndefined:
        return vk::ImageLayout::eUndefined;
    case ResourceState::kClearColor:
    case ResourceState::kClearDepth:
        return vk::ImageLayout::eTransferDstOptimal;
    case ResourceState::kUnorderedAccess:
        return vk::ImageLayout::eGeneral;
    case ResourceState::kPresent:
        return vk::ImageLayout::ePresentSrcKHR;
    case ResourceState::kRenderTarget:
        return vk::ImageLayout::eColorAttachmentOptimal;
    case ResourceState::kDepthTarget:
        return vk::ImageLayout::eDepthStencilAttachmentOptimal;
    case ResourceState::kPixelShaderResource:
    case ResourceState::kNonPixelShaderResource:
        return vk::ImageLayout::eShaderReadOnlyOptimal;
    case ResourceState::kCopyDest:
        return vk::ImageLayout::eTransferDstOptimal;
    case ResourceState::kCopySource:
        return vk::ImageLayout::eTransferSrcOptimal;
    case ResourceState::kShadingRateSource:
        return vk::ImageLayout::eShadingRateOptimalNV;
    default:
        assert(false);
        return vk::ImageLayout::eGeneral;
    }
}

void VKCommandList::ResourceBarrier(const std::vector<ResourceBarrierDesc>& barriers)
{
    std::vector<vk::ImageMemoryBarrier> image_memory_barriers;
    for (const auto& barrier : barriers)
    {
        if (!barrier.resource)
        {
            assert(false);
            continue;
        }

        decltype(auto) vk_resource = barrier.resource->As<VKResource>();
        VKResource::Image& image = vk_resource.image;
        if (!image.res)
            continue;

        vk::ImageLayout vk_state_before = ConvertSate(barrier.state_before);
        vk::ImageLayout vk_state_after = ConvertSate(barrier.state_after);
        if (vk_state_before == vk_state_after)
            continue;

        vk::ImageMemoryBarrier& image_memory_barrier = image_memory_barriers.emplace_back();
        image_memory_barrier.oldLayout = vk_state_before;
        image_memory_barrier.newLayout = vk_state_after;
        image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.image = image.res;

        vk::ImageSubresourceRange& range = image_memory_barrier.subresourceRange;
        range.aspectMask = m_device.GetAspectFlags(image.format);
        range.baseMipLevel = barrier.base_mip_level;
        range.levelCount = barrier.level_count;
        range.baseArrayLayer = barrier.base_array_layer;
        range.layerCount = barrier.layer_count;

        // Source layouts (old)
        // Source access mask controls actions that have to be finished on the old layout
        // before it will be transitioned to the new layout
        switch (image_memory_barrier.oldLayout)
        {
        case vk::ImageLayout::eUndefined:
            // Image layout is undefined (or does not matter)
            // Only valid as initial layout
            // No flags required, listed only for completeness
            image_memory_barrier.srcAccessMask = {};
            break;
        case vk::ImageLayout::ePreinitialized:
            // Image is preinitialized
            // Only valid as initial layout for linear images, preserves memory contents
            // Make sure host writes have been finished
            image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
            break;
        case vk::ImageLayout::eColorAttachmentOptimal:
            // Image is a color attachment
            // Make sure any writes to the color buffer have been finished
            image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
            break;
        case vk::ImageLayout::eDepthAttachmentOptimal:
            // Image is a depth/stencil attachment
            // Make sure any writes to the depth/stencil buffer have been finished
            image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            break;
        case vk::ImageLayout::eTransferSrcOptimal:
            // Image is a transfer source 
            // Make sure any reads from the image have been finished
            image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
            break;
        case vk::ImageLayout::eTransferDstOptimal:
            // Image is a transfer destination
            // Make sure any writes to the image have been finished
            image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            break;

        case vk::ImageLayout::eShaderReadOnlyOptimal:
            // Image is read by a shader
            // Make sure any shader reads from the image have been finished
            image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
            break;
        default:
            // Other source layouts aren't handled (yet)
            break;
        }

        // Target layouts (new)
        // Destination access mask controls the dependency for the new image layout
        switch (image_memory_barrier.newLayout)
        {
        case vk::ImageLayout::eTransferDstOptimal:
            // Image will be used as a transfer destination
            // Make sure any writes to the image have been finished
            image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
            break;

        case vk::ImageLayout::eTransferSrcOptimal:
            // Image will be used as a transfer source
            // Make sure any reads from the image have been finished
            image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
            break;

        case vk::ImageLayout::eColorAttachmentOptimal:
            // Image will be used as a color attachment
            // Make sure any writes to the color buffer have been finished
            image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
            break;

        case vk::ImageLayout::eDepthAttachmentOptimal:
            // Image layout will be used as a depth/stencil attachment
            // Make sure any writes to depth/stencil buffer have been finished
            image_memory_barrier.dstAccessMask = image_memory_barrier.dstAccessMask | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            break;

        case vk::ImageLayout::eShaderReadOnlyOptimal:
            // Image will be read in a shader (sampler, input attachment)
            // Make sure any writes to the image have been finished
            if (!image_memory_barrier.srcAccessMask)
            {
                image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eTransferWrite;
            }
            image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            break;
        default:
            // Other source layouts aren't handled (yet)
            break;
        }
    }

    if (!image_memory_barriers.empty())
    {
        m_command_list->pipelineBarrier(
            vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands,
            vk::DependencyFlagBits::eByRegion,
            0, nullptr,
            0, nullptr,
            image_memory_barriers.size(), image_memory_barriers.data());
    }
}

void VKCommandList::UAVResourceBarrier(const std::shared_ptr<Resource>& /*resource*/)
{
    vk::MemoryBarrier memory_barrier = {};
    memory_barrier.srcAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteNV
                                 | vk::AccessFlagBits::eAccelerationStructureReadNV
                                 | vk::AccessFlagBits::eShaderWrite
                                 | vk::AccessFlagBits::eShaderRead;
    memory_barrier.dstAccessMask = memory_barrier.srcAccessMask;
    m_command_list->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, vk::DependencyFlagBits::eByRegion, 1, &memory_barrier, 0, 0, 0, 0);
}

void VKCommandList::SetViewport(float x, float y, float width, float height)
{
    vk::Viewport viewport = {};
    viewport.x = 0;
    viewport.y = height - y;
    viewport.width = width;
    viewport.height = -height;
    viewport.minDepth = 0;
    viewport.maxDepth = 1.0;
    m_command_list->setViewport(0, 1, &viewport);
}

void VKCommandList::SetScissorRect(int32_t left, int32_t top, uint32_t right, uint32_t bottom)
{
    vk::Rect2D rect = {};
    rect.offset.x = left;
    rect.offset.y = top;
    rect.extent.width = right;
    rect.extent.height = bottom;
    m_command_list->setScissor(0, 1, &rect);
}

static vk::IndexType GetVkIndexType(gli::format format)
{
    vk::Format vk_format = static_cast<vk::Format>(format);
    switch (vk_format)
    {
    case vk::Format::eR16Uint:
        return vk::IndexType::eUint16;
    case vk::Format::eR32Uint:
        return vk::IndexType::eUint32;
    default:
        assert(false);
        return {};
    }
}

void VKCommandList::IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format)
{
    decltype(auto) vk_resource = resource->As<VKResource>();
    vk::IndexType index_type = GetVkIndexType(format);
    m_command_list->bindIndexBuffer(vk_resource.buffer.res.get(), 0, index_type);
}

void VKCommandList::IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource)
{
    decltype(auto) vk_resource = resource->As<VKResource>();
    vk::Buffer vertex_buffers[] = { vk_resource.buffer.res.get() };
    vk::DeviceSize offsets[] = { 0 };
    m_command_list->bindVertexBuffers(slot, 1, vertex_buffers, offsets);
}

void VKCommandList::RSSetShadingRate(ShadingRate shading_rate, const std::array<ShadingRateCombiner, 2>& combiners)
{
}

void VKCommandList::RSSetShadingRateImage(const std::shared_ptr<Resource>& resource)
{
}

void VKCommandList::BuildAccelerationStructure(vk::AccelerationStructureInfoNV& build_info, const vk::Buffer& instance_data, uint64_t instance_offset, const std::shared_ptr<Resource>& src, const std::shared_ptr<Resource>& dst, const std::shared_ptr<Resource>& scratch, uint64_t scratch_offset)
{
    decltype(auto) vk_dst = dst->As<VKResource>();
    decltype(auto) vk_scratch = scratch->As<VKResource>();

    vk::AccelerationStructureKHR vk_src_as = {};
    if (src)
    {
        decltype(auto) vk_src = src->As<VKResource>();
        vk_src_as = vk_src.as.acceleration_structure.get();
    }

    build_info.flags = vk_dst.as.flags;

    m_command_list->buildAccelerationStructureNV(
        build_info,
        instance_data,
        instance_offset,
        !!vk_src_as,
        vk_dst.as.acceleration_structure.get(),
        vk_src_as,
        vk_scratch.buffer.res.get(),
        scratch_offset
    );
}

void VKCommandList::BuildBottomLevelAS(const std::shared_ptr<Resource>& src, const std::shared_ptr<Resource>& dst,
                                       const std::shared_ptr<Resource>& scratch, uint64_t scratch_offset, const std::vector<RaytracingGeometryDesc>& descs)
{
    std::vector<vk::GeometryNV> geometry_descs;
    for (const auto& desc : descs)
    {
        geometry_descs.emplace_back(FillRaytracingGeometryDesc(desc.vertex, desc.index, desc.flags));
    }
    vk::AccelerationStructureInfoNV build_info = {};
    build_info.type = vk::AccelerationStructureTypeNV::eBottomLevel;
    build_info.geometryCount = geometry_descs.size();
    build_info.pGeometries = geometry_descs.data();
    BuildAccelerationStructure(build_info, {}, 0, src, dst, scratch, scratch_offset);
}

void VKCommandList::BuildTopLevelAS(const std::shared_ptr<Resource>& src, const std::shared_ptr<Resource>& dst, const std::shared_ptr<Resource>& scratch, uint64_t scratch_offset,
                                    const std::shared_ptr<Resource>& instance_data, uint64_t instance_offset, uint32_t instance_count)
{
    decltype(auto) vk_instance_data = instance_data->As<VKResource>();
    vk::AccelerationStructureInfoNV build_info = {};
    build_info.type = vk::AccelerationStructureTypeNV::eTopLevel;
    build_info.instanceCount = instance_count;
    BuildAccelerationStructure(build_info, vk_instance_data.buffer.res.get(), instance_offset, src, dst, scratch, scratch_offset);
}

void VKCommandList::CopyAccelerationStructure(const std::shared_ptr<Resource>& src, const std::shared_ptr<Resource>& dst, CopyAccelerationStructureMode mode)
{
    decltype(auto) vk_src = src->As<VKResource>();
    decltype(auto) vk_dst = dst->As<VKResource>();
    vk::CopyAccelerationStructureModeNV vk_mode = {};
    switch (mode)
    {
    case CopyAccelerationStructureMode::kClone:
        vk_mode = vk::CopyAccelerationStructureModeNV::eClone;
        break;
    case CopyAccelerationStructureMode::kCompact:
        vk_mode = vk::CopyAccelerationStructureModeNV::eCompact;
        break;
    default:
        assert(false);
    }
    m_command_list->copyAccelerationStructureNV(
        vk_dst.as.acceleration_structure.get(),
        vk_src.as.acceleration_structure.get(),
        vk_mode
    );
}

void VKCommandList::CopyBuffer(const std::shared_ptr<Resource>& src_buffer, const std::shared_ptr<Resource>& dst_buffer,
                               const std::vector<BufferCopyRegion>& regions)
{
    decltype(auto) vk_src_buffer = src_buffer->As<VKResource>();
    decltype(auto) vk_dst_buffer = dst_buffer->As<VKResource>();
    std::vector<vk::BufferCopy> vk_regions;
    for (const auto& region : regions)
    {
        vk_regions.emplace_back(region.src_offset, region.dst_offset, region.num_bytes);
    }
    m_command_list->copyBuffer(vk_src_buffer.buffer.res.get(), vk_dst_buffer.buffer.res.get(), vk_regions);
}

void VKCommandList::CopyBufferToTexture(const std::shared_ptr<Resource>& src_buffer, const std::shared_ptr<Resource>& dst_texture,
                                        const std::vector<BufferToTextureCopyRegion>& regions)
{
    decltype(auto) vk_src_buffer = src_buffer->As<VKResource>();
    decltype(auto) vk_dst_texture = dst_texture->As<VKResource>();
    std::vector<vk::BufferImageCopy> vk_regions;
    auto format = dst_texture->GetFormat();
    for (const auto& region : regions)
    {
        auto& vk_region = vk_regions.emplace_back();
        vk_region.bufferOffset = region.buffer_offset;
        if (gli::is_compressed(format))
        {
            auto extent = gli::block_extent(format);
            vk_region.bufferRowLength = region.buffer_row_pitch / gli::block_size(format) * extent.x;
            vk_region.bufferImageHeight = ((region.texture_extent.height + gli::block_extent(format).y - 1) / gli::block_extent(format).y) * extent.x;
        }
        else
        {
            vk_region.bufferRowLength = region.buffer_row_pitch / (gli::detail::bits_per_pixel(format) / 8);
            vk_region.bufferImageHeight = region.texture_extent.height;
        }
        vk_region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        vk_region.imageSubresource.mipLevel = region.texture_mip_level;
        vk_region.imageSubresource.baseArrayLayer = region.texture_array_layer;
        vk_region.imageSubresource.layerCount = 1;
        vk_region.imageOffset.x = region.texture_offset.x;
        vk_region.imageOffset.y = region.texture_offset.y;
        vk_region.imageOffset.z = region.texture_offset.z;
        vk_region.imageExtent.width = region.texture_extent.width;
        vk_region.imageExtent.height = region.texture_extent.height;
        vk_region.imageExtent.depth = region.texture_extent.depth;
    }
    m_command_list->copyBufferToImage(
        vk_src_buffer.buffer.res.get(),
        vk_dst_texture.image.res,
        vk::ImageLayout::eTransferDstOptimal,
        vk_regions);
}

void VKCommandList::CopyTexture(const std::shared_ptr<Resource>& src_texture, const std::shared_ptr<Resource>& dst_texture,
                                const std::vector<TextureCopyRegion>& regions)
{
    decltype(auto) vk_src_texture = src_texture->As<VKResource>();
    decltype(auto) vk_dst_texture = dst_texture->As<VKResource>();
    std::vector<vk::ImageCopy> vk_regions;
    for (const auto& region : regions)
    {
        auto& vk_region = vk_regions.emplace_back();
        vk_region.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        vk_region.srcSubresource.mipLevel = region.src_mip_level;
        vk_region.srcSubresource.baseArrayLayer = region.src_array_layer;
        vk_region.srcSubresource.layerCount = 1;
        vk_region.srcOffset.x = region.src_offset.x;
        vk_region.srcOffset.y = region.src_offset.y;
        vk_region.srcOffset.z = region.src_offset.z;

        vk_region.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        vk_region.dstSubresource.mipLevel = region.dst_mip_level;
        vk_region.dstSubresource.baseArrayLayer = region.dst_array_layer;
        vk_region.dstSubresource.layerCount = 1;
        vk_region.dstOffset.x = region.dst_offset.x;
        vk_region.dstOffset.y = region.dst_offset.y;
        vk_region.dstOffset.z = region.dst_offset.z;

        vk_region.extent.width = region.extent.width;
        vk_region.extent.height = region.extent.height;
        vk_region.extent.depth = region.extent.depth;
    }
    m_command_list->copyImage(vk_src_texture.image.res, vk::ImageLayout::eTransferSrcOptimal, vk_dst_texture.image.res, vk::ImageLayout::eTransferDstOptimal, vk_regions);
}

vk::CommandBuffer VKCommandList::GetCommandList()
{
    return m_command_list.get();
}
