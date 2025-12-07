#include "CommandList/VKCommandList.h"

#include "Adapter/VKAdapter.h"
#include "BindingSet/VKBindingSet.h"
#include "Device/VKDevice.h"
#include "Instance/VKInstance.h"
#include "Pipeline/VKComputePipeline.h"
#include "Pipeline/VKGraphicsPipeline.h"
#include "Pipeline/VKRayTracingPipeline.h"
#include "QueryHeap/VKQueryHeap.h"
#include "Resource/VKResource.h"
#include "Utilities/NotReached.h"
#include "Utilities/VKUtility.h"
#include "View/VKView.h"

#include <deque>

namespace {

vk::StridedDeviceAddressRegionKHR GetStridedDeviceAddressRegion(VKDevice& device, const RayTracingShaderTable& table)
{
    if (!table.resource) {
        return {};
    }
    decltype(auto) vk_resource = table.resource->As<VKResource>();
    vk::StridedDeviceAddressRegionKHR vk_table = {};
    vk_table.deviceAddress = device.GetDevice().getBufferAddress({ vk_resource.GetBuffer() }) + table.offset;
    vk_table.size = table.size;
    vk_table.stride = table.stride;
    return vk_table;
}

vk::IndexType GetVkIndexType(gli::format format)
{
    vk::Format vk_format = static_cast<vk::Format>(format);
    switch (vk_format) {
    case vk::Format::eR16Uint:
        return vk::IndexType::eUint16;
    case vk::Format::eR32Uint:
        return vk::IndexType::eUint32;
    default:
        NOTREACHED();
    }
}

vk::AttachmentLoadOp ConvertRenderPassLoadOp(RenderPassLoadOp op)
{
    switch (op) {
    case RenderPassLoadOp::kLoad:
        return vk::AttachmentLoadOp::eLoad;
    case RenderPassLoadOp::kClear:
        return vk::AttachmentLoadOp::eClear;
    case RenderPassLoadOp::kDontCare:
        return vk::AttachmentLoadOp::eDontCare;
    default:
        NOTREACHED();
    }
}

vk::AttachmentStoreOp ConvertRenderPassStoreOp(RenderPassStoreOp op)
{
    switch (op) {
    case RenderPassStoreOp::kStore:
        return vk::AttachmentStoreOp::eStore;
    case RenderPassStoreOp::kDontCare:
        return vk::AttachmentStoreOp::eDontCare;
    default:
        NOTREACHED();
    }
}

} // namespace

VKCommandList::VKCommandList(VKDevice& device, CommandListType type)
    : device_(device)
{
    vk::CommandBufferAllocateInfo cmd_buf_alloc_info = {};
    cmd_buf_alloc_info.commandPool = device.GetCmdPool(type);
    cmd_buf_alloc_info.commandBufferCount = 1;
    cmd_buf_alloc_info.level = vk::CommandBufferLevel::ePrimary;
    std::vector<vk::UniqueCommandBuffer> cmd_bufs = device.GetDevice().allocateCommandBuffersUnique(cmd_buf_alloc_info);
    command_list_ = std::move(cmd_bufs.front());
    vk::CommandBufferBeginInfo begin_info = {};
    command_list_->begin(begin_info);
}

void VKCommandList::Reset()
{
    Close();
    vk::CommandBufferBeginInfo begin_info = {};
    command_list_->begin(begin_info);
    closed_ = false;
    state_.reset();
    binding_set_.reset();
}

void VKCommandList::Close()
{
    if (!closed_) {
        command_list_->end();
        closed_ = true;
    }
}

vk::PipelineBindPoint GetPipelineBindPoint(PipelineType type)
{
    switch (type) {
    case PipelineType::kGraphics:
        return vk::PipelineBindPoint::eGraphics;
    case PipelineType::kCompute:
        return vk::PipelineBindPoint::eCompute;
    case PipelineType::kRayTracing:
        return vk::PipelineBindPoint::eRayTracingKHR;
    default:
        NOTREACHED();
    }
}

void VKCommandList::BindPipeline(const std::shared_ptr<Pipeline>& state)
{
    if (state == state_) {
        return;
    }
    state_ = std::static_pointer_cast<VKPipeline>(state);
    command_list_->bindPipeline(GetPipelineBindPoint(state_->GetPipelineType()), state_->GetPipeline());
}

void VKCommandList::BindBindingSet(const std::shared_ptr<BindingSet>& binding_set)
{
    if (binding_set == binding_set_) {
        return;
    }
    binding_set_ = binding_set;
    decltype(auto) vk_binding_set = binding_set->As<VKBindingSet>();
    decltype(auto) descriptor_sets = vk_binding_set.GetDescriptorSets();
    if (descriptor_sets.empty()) {
        return;
    }
    command_list_->bindDescriptorSets(GetPipelineBindPoint(state_->GetPipelineType()), state_->GetPipelineLayout(), 0,
                                      descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);
}

void VKCommandList::BeginRenderPass(const RenderPassDesc& render_pass_desc)
{
    auto get_image_view = [&](const std::shared_ptr<View>& view) -> vk::ImageView {
        if (!view) {
            return {};
        }
        return view->As<VKView>().GetImageView();
    };

    std::vector<vk::RenderingAttachmentInfo> color_attachments(render_pass_desc.colors.size());
    for (size_t i = 0; i < render_pass_desc.colors.size(); ++i) {
        vk::RenderingAttachmentInfo& color_attachment = color_attachments[i];
        color_attachment.imageView = get_image_view(render_pass_desc.colors[i].view);
        color_attachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
        color_attachment.loadOp = ConvertRenderPassLoadOp(render_pass_desc.colors[i].load_op);
        color_attachment.storeOp = ConvertRenderPassStoreOp(render_pass_desc.colors[i].store_op);
        if (render_pass_desc.colors[i].load_op == RenderPassLoadOp::kClear) {
            color_attachment.clearValue.color.float32[0] = render_pass_desc.colors[i].clear_value.r;
            color_attachment.clearValue.color.float32[1] = render_pass_desc.colors[i].clear_value.g;
            color_attachment.clearValue.color.float32[2] = render_pass_desc.colors[i].clear_value.b;
            color_attachment.clearValue.color.float32[3] = render_pass_desc.colors[i].clear_value.a;
        }
    }

    vk::RenderingAttachmentInfo depth_attachment = {};
    if (render_pass_desc.depth_stencil_view &&
        gli::is_depth(render_pass_desc.depth_stencil_view->GetResource()->GetFormat())) {
        depth_attachment.imageView = get_image_view(render_pass_desc.depth_stencil_view);
        depth_attachment.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    }
    depth_attachment.loadOp = ConvertRenderPassLoadOp(render_pass_desc.depth.load_op);
    depth_attachment.storeOp = ConvertRenderPassStoreOp(render_pass_desc.depth.store_op);
    depth_attachment.clearValue.depthStencil.depth = render_pass_desc.depth.clear_value;

    vk::RenderingAttachmentInfo stencil_attachment = {};
    if (render_pass_desc.depth_stencil_view &&
        gli::is_stencil(render_pass_desc.depth_stencil_view->GetResource()->GetFormat())) {
        stencil_attachment.imageView = get_image_view(render_pass_desc.depth_stencil_view);
        stencil_attachment.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    }
    stencil_attachment.loadOp = ConvertRenderPassLoadOp(render_pass_desc.stencil.load_op);
    stencil_attachment.storeOp = ConvertRenderPassStoreOp(render_pass_desc.stencil.store_op);
    stencil_attachment.clearValue.depthStencil.stencil = render_pass_desc.stencil.clear_value;

    vk::RenderingInfo rendering_info = {};
    rendering_info.renderArea = { static_cast<int32_t>(render_pass_desc.render_area.x),
                                  static_cast<int32_t>(render_pass_desc.render_area.y),
                                  render_pass_desc.render_area.width, render_pass_desc.render_area.height };
    rendering_info.layerCount = render_pass_desc.layers;
    rendering_info.colorAttachmentCount = color_attachments.size();
    rendering_info.pColorAttachments = color_attachments.data();
    rendering_info.pDepthAttachment = &depth_attachment;
    rendering_info.pStencilAttachment = &stencil_attachment;

    vk::RenderingFragmentShadingRateAttachmentInfoKHR fragment_shading_rate_attachment = {};
    if (render_pass_desc.shading_rate_image_view) {
        fragment_shading_rate_attachment.imageView = get_image_view(render_pass_desc.shading_rate_image_view);
        fragment_shading_rate_attachment.imageLayout = vk::ImageLayout::eFragmentShadingRateAttachmentOptimalKHR;
        fragment_shading_rate_attachment.shadingRateAttachmentTexelSize.width = device_.GetShadingRateImageTileSize();
        fragment_shading_rate_attachment.shadingRateAttachmentTexelSize.height = device_.GetShadingRateImageTileSize();
        rendering_info.pNext = &fragment_shading_rate_attachment;
    }

    command_list_->beginRendering(&rendering_info);
}

void VKCommandList::EndRenderPass()
{
    command_list_->endRendering();
}

void VKCommandList::BeginEvent(const std::string& name)
{
    if (device_.GetAdapter().GetInstance().IsDebugUtilsSupported()) {
        vk::DebugUtilsLabelEXT label = {};
        label.pLabelName = name.c_str();
        command_list_->beginDebugUtilsLabelEXT(&label);
    }
}

void VKCommandList::EndEvent()
{
    if (device_.GetAdapter().GetInstance().IsDebugUtilsSupported()) {
        command_list_->endDebugUtilsLabelEXT();
    }
}

void VKCommandList::Draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
{
    command_list_->draw(vertex_count, instance_count, first_vertex, first_instance);
}

void VKCommandList::DrawIndexed(uint32_t index_count,
                                uint32_t instance_count,
                                uint32_t first_index,
                                int32_t vertex_offset,
                                uint32_t first_instance)
{
    command_list_->drawIndexed(index_count, instance_count, first_index, vertex_offset, first_instance);
}

void VKCommandList::DrawIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset)
{
    DrawIndirectCount(argument_buffer, argument_buffer_offset, {}, 0, 1, sizeof(DrawIndirectCommand));
}

void VKCommandList::DrawIndexedIndirect(const std::shared_ptr<Resource>& argument_buffer,
                                        uint64_t argument_buffer_offset)
{
    DrawIndexedIndirectCount(argument_buffer, argument_buffer_offset, {}, 0, 1, sizeof(DrawIndexedIndirectCommand));
}

void VKCommandList::DrawIndirectCount(const std::shared_ptr<Resource>& argument_buffer,
                                      uint64_t argument_buffer_offset,
                                      const std::shared_ptr<Resource>& count_buffer,
                                      uint64_t count_buffer_offset,
                                      uint32_t max_draw_count,
                                      uint32_t stride)
{
    decltype(auto) vk_argument_buffer = argument_buffer->As<VKResource>();
    if (count_buffer) {
        decltype(auto) vk_count_buffer = count_buffer->As<VKResource>();
        command_list_->drawIndirectCount(vk_argument_buffer.GetBuffer(), argument_buffer_offset,
                                         vk_count_buffer.GetBuffer(), count_buffer_offset, max_draw_count, stride);
    } else {
        assert(count_buffer_offset == 0);
        command_list_->drawIndirect(vk_argument_buffer.GetBuffer(), argument_buffer_offset, max_draw_count, stride);
    }
}

void VKCommandList::DrawIndexedIndirectCount(const std::shared_ptr<Resource>& argument_buffer,
                                             uint64_t argument_buffer_offset,
                                             const std::shared_ptr<Resource>& count_buffer,
                                             uint64_t count_buffer_offset,
                                             uint32_t max_draw_count,
                                             uint32_t stride)
{
    decltype(auto) vk_argument_buffer = argument_buffer->As<VKResource>();
    if (count_buffer) {
        decltype(auto) vk_count_buffer = count_buffer->As<VKResource>();
        command_list_->drawIndexedIndirectCount(vk_argument_buffer.GetBuffer(), argument_buffer_offset,
                                                vk_count_buffer.GetBuffer(), count_buffer_offset, max_draw_count,
                                                stride);
    } else {
        assert(count_buffer_offset == 0);
        command_list_->drawIndexedIndirect(vk_argument_buffer.GetBuffer(), argument_buffer_offset, max_draw_count,
                                           stride);
    }
}

void VKCommandList::Dispatch(uint32_t thread_group_count_x,
                             uint32_t thread_group_count_y,
                             uint32_t thread_group_count_z)
{
    command_list_->dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z);
}

void VKCommandList::DispatchIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset)
{
    decltype(auto) vk_argument_buffer = argument_buffer->As<VKResource>();
    command_list_->dispatchIndirect(vk_argument_buffer.GetBuffer(), argument_buffer_offset);
}

void VKCommandList::DispatchMesh(uint32_t thread_group_count_x,
                                 uint32_t thread_group_count_y,
                                 uint32_t thread_group_count_z)
{
    command_list_->drawMeshTasksEXT(thread_group_count_x, thread_group_count_y, thread_group_count_z);
}

void VKCommandList::DispatchRays(const RayTracingShaderTables& shader_tables,
                                 uint32_t width,
                                 uint32_t height,
                                 uint32_t depth)
{
    command_list_->traceRaysKHR(GetStridedDeviceAddressRegion(device_, shader_tables.raygen),
                                GetStridedDeviceAddressRegion(device_, shader_tables.miss),
                                GetStridedDeviceAddressRegion(device_, shader_tables.hit),
                                GetStridedDeviceAddressRegion(device_, shader_tables.callable), width, height, depth);
}

void VKCommandList::ResourceBarrier(const std::vector<ResourceBarrierDesc>& barriers)
{
    std::vector<vk::ImageMemoryBarrier> image_memory_barriers;
    for (const auto& barrier : barriers) {
        if (!barrier.resource) {
            assert(false);
            continue;
        }

        decltype(auto) vk_resource = barrier.resource->As<VKResource>();
        const vk::Image& image = vk_resource.GetImage();
        if (!image) {
            continue;
        }

        vk::ImageLayout vk_state_before = ConvertState(barrier.state_before);
        vk::ImageLayout vk_state_after = ConvertState(barrier.state_after);
        if (vk_state_before == vk_state_after) {
            continue;
        }

        vk::ImageMemoryBarrier& image_memory_barrier = image_memory_barriers.emplace_back();
        image_memory_barrier.oldLayout = vk_state_before;
        image_memory_barrier.newLayout = vk_state_after;
        image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.image = image;

        vk::ImageSubresourceRange& range = image_memory_barrier.subresourceRange;
        range.aspectMask = device_.GetAspectFlags(static_cast<vk::Format>(vk_resource.GetFormat()));
        range.baseMipLevel = barrier.base_mip_level;
        range.levelCount = barrier.level_count;
        range.baseArrayLayer = barrier.base_array_layer;
        range.layerCount = barrier.layer_count;

        // Source layouts (old)
        // Source access mask controls actions that have to be finished on the old layout
        // before it will be transitioned to the new layout
        switch (image_memory_barrier.oldLayout) {
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
        case vk::ImageLayout::eFragmentShadingRateAttachmentOptimalKHR:
            image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eFragmentShadingRateAttachmentReadKHR;
        default:
            // Other source layouts aren't handled (yet)
            break;
        }

        // Target layouts (new)
        // Destination access mask controls the dependency for the new image layout
        switch (image_memory_barrier.newLayout) {
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
            image_memory_barrier.dstAccessMask =
                image_memory_barrier.dstAccessMask | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            break;

        case vk::ImageLayout::eShaderReadOnlyOptimal:
            // Image will be read in a shader (sampler, input attachment)
            // Make sure any writes to the image have been finished
            if (!image_memory_barrier.srcAccessMask) {
                image_memory_barrier.srcAccessMask =
                    vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eTransferWrite;
            }
            image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            break;
        case vk::ImageLayout::eFragmentShadingRateAttachmentOptimalKHR:
            image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eFragmentShadingRateAttachmentReadKHR;
            break;
        default:
            // Other source layouts aren't handled (yet)
            break;
        }
    }

    if (!image_memory_barriers.empty()) {
        command_list_->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands,
                                       vk::DependencyFlagBits::eByRegion, 0, nullptr, 0, nullptr,
                                       image_memory_barriers.size(), image_memory_barriers.data());
    }
}

void VKCommandList::UAVResourceBarrier(const std::shared_ptr<Resource>& /*resource*/)
{
    vk::MemoryBarrier memory_barrier = {};
    memory_barrier.srcAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteKHR |
                                   vk::AccessFlagBits::eAccelerationStructureReadKHR |
                                   vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead;
    memory_barrier.dstAccessMask = memory_barrier.srcAccessMask;
    command_list_->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands,
                                   vk::DependencyFlagBits::eByRegion, 1, &memory_barrier, 0, nullptr, 0, nullptr);
}

void VKCommandList::SetViewport(float x, float y, float width, float height, float min_depth, float max_depth)
{
    vk::Viewport viewport = {};
    viewport.x = 0;
    viewport.y = height - y;
    viewport.width = width;
    viewport.height = -height;
    viewport.minDepth = min_depth;
    viewport.maxDepth = max_depth;
    command_list_->setViewport(0, 1, &viewport);
}

void VKCommandList::SetScissorRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
{
    vk::Rect2D rect = {};
    rect.offset.x = left;
    rect.offset.y = top;
    rect.extent.width = right - left;
    rect.extent.height = bottom - top;
    command_list_->setScissor(0, 1, &rect);
}

void VKCommandList::IASetIndexBuffer(const std::shared_ptr<Resource>& resource, uint64_t offset, gli::format format)
{
    decltype(auto) vk_resource = resource->As<VKResource>();
    vk::IndexType index_type = GetVkIndexType(format);
    command_list_->bindIndexBuffer(vk_resource.GetBuffer(), offset, index_type);
}

void VKCommandList::IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource, uint64_t offset)
{
    decltype(auto) vk_resource = resource->As<VKResource>();
    vk::Buffer buffers[] = { vk_resource.GetBuffer() };
    vk::DeviceSize offsets[] = { offset };
    command_list_->bindVertexBuffers(slot, 1, buffers, offsets);
}

void VKCommandList::RSSetShadingRate(ShadingRate shading_rate, const std::array<ShadingRateCombiner, 2>& combiners)
{
    vk::Extent2D fragment_size = ConvertShadingRate(shading_rate);
    std::array<vk::FragmentShadingRateCombinerOpKHR, 2> vk_combiners = ConvertShadingRateCombiners(combiners);
    command_list_->setFragmentShadingRateKHR(&fragment_size, vk_combiners.data());
}

void VKCommandList::SetDepthBounds(float min_depth_bounds, float max_depth_bounds)
{
    command_list_->setDepthBounds(min_depth_bounds, max_depth_bounds);
}

void VKCommandList::SetStencilReference(uint32_t stencil_reference)
{
    command_list_->setStencilReference(vk::StencilFaceFlagBits::eFrontAndBack, stencil_reference);
}

void VKCommandList::BuildBottomLevelAS(const std::shared_ptr<Resource>& src,
                                       const std::shared_ptr<Resource>& dst,
                                       const std::shared_ptr<Resource>& scratch,
                                       uint64_t scratch_offset,
                                       const std::vector<RaytracingGeometryDesc>& descs,
                                       BuildAccelerationStructureFlags flags)
{
    std::vector<vk::AccelerationStructureGeometryKHR> geometry_descs;
    for (const auto& desc : descs) {
        geometry_descs.emplace_back(device_.FillRaytracingGeometryTriangles(desc.vertex, desc.index, desc.flags));
    }

    decltype(auto) vk_dst = dst->As<VKResource>();
    decltype(auto) vk_scratch = scratch->As<VKResource>();

    vk::AccelerationStructureKHR vk_src_as = {};
    if (src) {
        decltype(auto) vk_src = src->As<VKResource>();
        vk_src_as = vk_src.GetAccelerationStructure();
    }

    std::vector<vk::AccelerationStructureBuildRangeInfoKHR> ranges;
    for (const auto& desc : descs) {
        vk::AccelerationStructureBuildRangeInfoKHR& offset = ranges.emplace_back();
        if (desc.index.res) {
            offset.primitiveCount = desc.index.count / 3;
        } else {
            offset.primitiveCount = desc.vertex.count / 3;
        }
    }
    std::vector<const vk::AccelerationStructureBuildRangeInfoKHR*> range_infos(ranges.size());
    for (size_t i = 0; i < ranges.size(); ++i) {
        range_infos[i] = &ranges[i];
    }

    vk::AccelerationStructureBuildGeometryInfoKHR infos = {};
    infos.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
    infos.flags = Convert(flags);
    infos.dstAccelerationStructure = vk_dst.GetAccelerationStructure();
    infos.srcAccelerationStructure = vk_src_as;
    if (vk_src_as) {
        infos.mode = vk::BuildAccelerationStructureModeKHR::eUpdate;
    } else {
        infos.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
    }
    infos.scratchData = device_.GetDevice().getBufferAddress({ vk_scratch.GetBuffer() }) + scratch_offset;
    infos.pGeometries = geometry_descs.data();
    infos.geometryCount = geometry_descs.size();

    command_list_->buildAccelerationStructuresKHR(1, &infos, range_infos.data());
}

void VKCommandList::BuildTopLevelAS(const std::shared_ptr<Resource>& src,
                                    const std::shared_ptr<Resource>& dst,
                                    const std::shared_ptr<Resource>& scratch,
                                    uint64_t scratch_offset,
                                    const std::shared_ptr<Resource>& instance_data,
                                    uint64_t instance_offset,
                                    uint32_t instance_count,
                                    BuildAccelerationStructureFlags flags)
{
    decltype(auto) vk_instance_data = instance_data->As<VKResource>();
    vk::DeviceAddress instance_address = {};
    instance_address = device_.GetDevice().getBufferAddress(vk_instance_data.GetBuffer()) + instance_offset;
    vk::AccelerationStructureGeometryKHR top_as_geometry = {};
    top_as_geometry.geometryType = vk::GeometryTypeKHR::eInstances;
    top_as_geometry.geometry.setInstances({});
    top_as_geometry.geometry.instances.arrayOfPointers = VK_FALSE;
    top_as_geometry.geometry.instances.data = instance_address;

    decltype(auto) vk_dst = dst->As<VKResource>();
    decltype(auto) vk_scratch = scratch->As<VKResource>();

    vk::AccelerationStructureKHR vk_src_as = {};
    if (src) {
        decltype(auto) vk_src = src->As<VKResource>();
        vk_src_as = vk_src.GetAccelerationStructure();
    }

    vk::AccelerationStructureBuildRangeInfoKHR acceleration_structure_build_range_info = {};
    acceleration_structure_build_range_info.primitiveCount = instance_count;
    std::vector<vk::AccelerationStructureBuildRangeInfoKHR*> offset_infos = {
        &acceleration_structure_build_range_info
    };

    vk::AccelerationStructureBuildGeometryInfoKHR infos = {};
    infos.type = vk::AccelerationStructureTypeKHR::eTopLevel;
    infos.flags = Convert(flags);
    infos.dstAccelerationStructure = vk_dst.GetAccelerationStructure();
    infos.srcAccelerationStructure = vk_src_as;
    if (vk_src_as) {
        infos.mode = vk::BuildAccelerationStructureModeKHR::eUpdate;
    } else {
        infos.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
    }
    infos.scratchData = device_.GetDevice().getBufferAddress({ vk_scratch.GetBuffer() }) + scratch_offset;
    infos.pGeometries = &top_as_geometry;
    infos.geometryCount = 1;

    command_list_->buildAccelerationStructuresKHR(1, &infos, offset_infos.data());
}

void VKCommandList::CopyAccelerationStructure(const std::shared_ptr<Resource>& src,
                                              const std::shared_ptr<Resource>& dst,
                                              CopyAccelerationStructureMode mode)
{
    decltype(auto) vk_src = src->As<VKResource>();
    decltype(auto) vk_dst = dst->As<VKResource>();
    vk::CopyAccelerationStructureInfoKHR info = {};
    switch (mode) {
    case CopyAccelerationStructureMode::kClone:
        info.mode = vk::CopyAccelerationStructureModeKHR::eClone;
        break;
    case CopyAccelerationStructureMode::kCompact:
        info.mode = vk::CopyAccelerationStructureModeKHR::eCompact;
        break;
    default:
        NOTREACHED();
    }
    info.dst = vk_dst.GetAccelerationStructure();
    info.src = vk_src.GetAccelerationStructure();
    command_list_->copyAccelerationStructureKHR(info);
}

void VKCommandList::CopyBuffer(const std::shared_ptr<Resource>& src_buffer,
                               const std::shared_ptr<Resource>& dst_buffer,
                               const std::vector<BufferCopyRegion>& regions)
{
    decltype(auto) vk_src_buffer = src_buffer->As<VKResource>();
    decltype(auto) vk_dst_buffer = dst_buffer->As<VKResource>();
    std::vector<vk::BufferCopy> vk_regions;
    for (const auto& region : regions) {
        vk_regions.emplace_back(region.src_offset, region.dst_offset, region.num_bytes);
    }
    command_list_->copyBuffer(vk_src_buffer.GetBuffer(), vk_dst_buffer.GetBuffer(), vk_regions);
}

void VKCommandList::CopyBufferToTexture(const std::shared_ptr<Resource>& src_buffer,
                                        const std::shared_ptr<Resource>& dst_texture,
                                        const std::vector<BufferToTextureCopyRegion>& regions)
{
    decltype(auto) vk_src_buffer = src_buffer->As<VKResource>();
    decltype(auto) vk_dst_texture = dst_texture->As<VKResource>();
    std::vector<vk::BufferImageCopy> vk_regions;
    auto format = dst_texture->GetFormat();
    for (const auto& region : regions) {
        auto& vk_region = vk_regions.emplace_back();
        vk_region.bufferOffset = region.buffer_offset;
        if (gli::is_compressed(format)) {
            auto extent = gli::block_extent(format);
            vk_region.bufferRowLength = region.buffer_row_pitch / gli::block_size(format) * extent.x;
            vk_region.bufferImageHeight =
                ((region.texture_extent.height + gli::block_extent(format).y - 1) / gli::block_extent(format).y) *
                extent.x;
        } else {
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
    command_list_->copyBufferToImage(vk_src_buffer.GetBuffer(), vk_dst_texture.GetImage(),
                                     vk::ImageLayout::eTransferDstOptimal, vk_regions);
}

void VKCommandList::CopyTexture(const std::shared_ptr<Resource>& src_texture,
                                const std::shared_ptr<Resource>& dst_texture,
                                const std::vector<TextureCopyRegion>& regions)
{
    decltype(auto) vk_src_texture = src_texture->As<VKResource>();
    decltype(auto) vk_dst_texture = dst_texture->As<VKResource>();
    std::vector<vk::ImageCopy> vk_regions;
    for (const auto& region : regions) {
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
    command_list_->copyImage(vk_src_texture.GetImage(), vk::ImageLayout::eTransferSrcOptimal, vk_dst_texture.GetImage(),
                             vk::ImageLayout::eTransferDstOptimal, vk_regions);
}

void VKCommandList::WriteAccelerationStructuresProperties(
    const std::vector<std::shared_ptr<Resource>>& acceleration_structures,
    const std::shared_ptr<QueryHeap>& query_heap,
    uint32_t first_query)
{
    std::vector<vk::AccelerationStructureKHR> vk_acceleration_structures;
    vk_acceleration_structures.reserve(acceleration_structures.size());
    for (const auto& acceleration_structure : acceleration_structures) {
        vk_acceleration_structures.emplace_back(acceleration_structure->As<VKResource>().GetAccelerationStructure());
    }
    decltype(auto) vk_query_heap = query_heap->As<VKQueryHeap>();
    auto query_type = vk_query_heap.GetQueryType();
    assert(query_type == vk::QueryType::eAccelerationStructureCompactedSizeKHR);
    command_list_->resetQueryPool(vk_query_heap.GetQueryPool(), first_query, acceleration_structures.size());
    command_list_->writeAccelerationStructuresPropertiesKHR(vk_acceleration_structures.size(),
                                                            vk_acceleration_structures.data(), query_type,
                                                            vk_query_heap.GetQueryPool(), first_query);
}

void VKCommandList::ResolveQueryData(const std::shared_ptr<QueryHeap>& query_heap,
                                     uint32_t first_query,
                                     uint32_t query_count,
                                     const std::shared_ptr<Resource>& dst_buffer,
                                     uint64_t dst_offset)
{
    decltype(auto) vk_query_heap = query_heap->As<VKQueryHeap>();
    auto query_type = vk_query_heap.GetQueryType();
    assert(query_type == vk::QueryType::eAccelerationStructureCompactedSizeKHR);
    command_list_->copyQueryPoolResults(vk_query_heap.GetQueryPool(), first_query, query_count,
                                        dst_buffer->As<VKResource>().GetBuffer(), dst_offset, sizeof(uint64_t),
                                        vk::QueryResultFlagBits::eWait);
}

void VKCommandList::SetName(const std::string& name)
{
    vk::DebugUtilsObjectNameInfoEXT info = {};
    info.pObjectName = name.c_str();
    info.objectType = vk::ObjectType::eCommandBuffer;
    info.objectHandle = reinterpret_cast<uint64_t>(static_cast<VkCommandBuffer>(command_list_.get()));
    device_.GetDevice().setDebugUtilsObjectNameEXT(info);
}

vk::CommandBuffer VKCommandList::GetCommandList()
{
    return command_list_.get();
}
