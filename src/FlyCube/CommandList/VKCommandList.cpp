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

class VKFramebuffer {
public:
    VKFramebuffer(VKDevice& device, const FramebufferDesc& desc, vk::RenderPass render_pass)
    {
        vk::FramebufferCreateInfo framebuffer_info = {};
        framebuffer_info.width = desc.width;
        framebuffer_info.height = desc.height;
        framebuffer_info.layers = std::numeric_limits<uint32_t>::max();

        std::deque<vk::Format> formats;
        std::vector<vk::FramebufferAttachmentImageInfo> attachment_image_infos = {};
        auto add_view = [&](const std::shared_ptr<View>& view) {
            if (!view) {
                return;
            }
            decltype(auto) vk_view = view->As<VKView>();
            decltype(auto) resource = vk_view.GetResource();
            if (!resource) {
                return;
            }
            decltype(auto) vk_resource = resource->As<VKResource>();

            formats.push_back(static_cast<vk::Format>(vk_resource.GetFormat()));

            vk::FramebufferAttachmentImageInfo frame_buffer_attachment_image_info = {};
            frame_buffer_attachment_image_info.flags = vk_resource.GetImageCreateFlags();
            frame_buffer_attachment_image_info.usage = vk_resource.GetImageUsageFlags();
            frame_buffer_attachment_image_info.width = vk_resource.GetWidth() >> vk_view.GetBaseMipLevel();
            frame_buffer_attachment_image_info.height = vk_resource.GetHeight() >> vk_view.GetBaseMipLevel();
            frame_buffer_attachment_image_info.layerCount = vk_view.GetLayerCount();
            frame_buffer_attachment_image_info.viewFormatCount = 1;
            frame_buffer_attachment_image_info.pViewFormats = &formats.back();
            attachment_image_infos.push_back(frame_buffer_attachment_image_info);

            m_attachments.emplace_back(vk_view.GetImageView());

            assert(frame_buffer_attachment_image_info.width >= framebuffer_info.width);
            assert(frame_buffer_attachment_image_info.height >= framebuffer_info.height);
            framebuffer_info.layers = std::min(framebuffer_info.layers, frame_buffer_attachment_image_info.layerCount);
        };
        for (auto& rtv : desc.colors) {
            add_view(rtv);
        }
        add_view(desc.depth_stencil);
        add_view(desc.shading_rate_image);

        if (framebuffer_info.layers == std::numeric_limits<uint32_t>::max()) {
            framebuffer_info.layers = 1;
        }

        framebuffer_info.renderPass = render_pass;
        framebuffer_info.flags = vk::FramebufferCreateFlagBits::eImageless;
        framebuffer_info.attachmentCount = attachment_image_infos.size();
        vk::FramebufferAttachmentsCreateInfo framebuffer_attachments_info = {};
        framebuffer_attachments_info.attachmentImageInfoCount = attachment_image_infos.size();
        framebuffer_attachments_info.pAttachmentImageInfos = attachment_image_infos.data();
        framebuffer_info.pNext = &framebuffer_attachments_info;
        m_framebuffer = device.GetDevice().createFramebufferUnique(framebuffer_info);
    }

    const std::vector<vk::ImageView>& GetAttachments() const
    {
        return m_attachments;
    }

    vk::UniqueFramebuffer TakeFramebuffer()
    {
        return std::move(m_framebuffer);
    }

private:
    vk::UniqueFramebuffer m_framebuffer;
    std::vector<vk::ImageView> m_attachments;
};

} // namespace

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
    m_command_list->begin(begin_info);
}

void VKCommandList::Reset()
{
    Close();
    vk::CommandBufferBeginInfo begin_info = {};
    m_command_list->begin(begin_info);
    m_closed = false;
    m_state.reset();
    m_binding_set.reset();
}

void VKCommandList::Close()
{
    if (!m_closed) {
        m_command_list->end();
        m_closed = true;
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
    if (state == m_state) {
        return;
    }
    m_state = std::static_pointer_cast<VKPipeline>(state);
    m_command_list->bindPipeline(GetPipelineBindPoint(m_state->GetPipelineType()), m_state->GetPipeline());
}

void VKCommandList::BindBindingSet(const std::shared_ptr<BindingSet>& binding_set)
{
    if (binding_set == m_binding_set) {
        return;
    }
    m_binding_set = binding_set;
    decltype(auto) vk_binding_set = binding_set->As<VKBindingSet>();
    decltype(auto) descriptor_sets = vk_binding_set.GetDescriptorSets();
    if (descriptor_sets.empty()) {
        return;
    }
    m_command_list->bindDescriptorSets(GetPipelineBindPoint(m_state->GetPipelineType()), m_state->GetPipelineLayout(),
                                       0, descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);
}

void VKCommandList::BeginRenderPass(const std::shared_ptr<RenderPass>& render_pass,
                                    const FramebufferDesc& framebuffer_desc,
                                    const ClearDesc& clear_desc)
{
    if (m_device.IsDynamicRenderingSupported()) {
        const auto& render_pass_desc = render_pass->As<RenderPassBase>().GetDesc();

        uint32_t layers = std::numeric_limits<uint32_t>::max();
        auto get_image_view = [&](const std::shared_ptr<View>& view) -> vk::ImageView {
            if (!view) {
                return {};
            }
            decltype(auto) vk_view = view->As<VKView>();
            layers = std::min(layers, vk_view.GetLayerCount());
            return vk_view.GetImageView();
        };

        std::vector<vk::RenderingAttachmentInfo> color_attachments(render_pass_desc.colors.size());
        for (size_t i = 0; i < render_pass_desc.colors.size(); ++i) {
            vk::RenderingAttachmentInfo& color_attachment = color_attachments[i];
            color_attachment.imageView = get_image_view(framebuffer_desc.colors[i]);
            color_attachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
            color_attachment.loadOp = ConvertRenderPassLoadOp(render_pass_desc.colors[i].load_op);
            color_attachment.storeOp = ConvertRenderPassStoreOp(render_pass_desc.colors[i].store_op);
            if (i < clear_desc.colors.size()) {
                color_attachment.clearValue.color.float32[0] = clear_desc.colors[i].r;
                color_attachment.clearValue.color.float32[1] = clear_desc.colors[i].g;
                color_attachment.clearValue.color.float32[2] = clear_desc.colors[i].b;
                color_attachment.clearValue.color.float32[3] = clear_desc.colors[i].a;
            }
        }

        vk::RenderingAttachmentInfo depth_attachment = {};
        if (render_pass_desc.depth_stencil.format != gli::format::FORMAT_UNDEFINED &&
            gli::is_depth(render_pass_desc.depth_stencil.format)) {
            depth_attachment.imageView = get_image_view(framebuffer_desc.depth_stencil);
            depth_attachment.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        }
        depth_attachment.loadOp = ConvertRenderPassLoadOp(render_pass_desc.depth_stencil.depth_load_op);
        depth_attachment.storeOp = ConvertRenderPassStoreOp(render_pass_desc.depth_stencil.depth_store_op);
        depth_attachment.clearValue.depthStencil.depth = clear_desc.depth;

        vk::RenderingAttachmentInfo stencil_attachment = {};
        if (render_pass_desc.depth_stencil.format != gli::format::FORMAT_UNDEFINED &&
            gli::is_stencil(render_pass_desc.depth_stencil.format)) {
            stencil_attachment.imageView = get_image_view(framebuffer_desc.depth_stencil);
            stencil_attachment.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        }
        stencil_attachment.loadOp = ConvertRenderPassLoadOp(render_pass_desc.depth_stencil.stencil_load_op);
        stencil_attachment.storeOp = ConvertRenderPassStoreOp(render_pass_desc.depth_stencil.stencil_store_op);
        stencil_attachment.clearValue.depthStencil.stencil = clear_desc.stencil;

        if (layers == std::numeric_limits<uint32_t>::max()) {
            layers = 1;
        }

        vk::RenderingInfo rendering_info = {};
        rendering_info.renderArea.extent.width = framebuffer_desc.width;
        rendering_info.renderArea.extent.height = framebuffer_desc.height;
        rendering_info.layerCount = layers;
        rendering_info.colorAttachmentCount = color_attachments.size();
        rendering_info.pColorAttachments = color_attachments.data();
        rendering_info.pDepthAttachment = &depth_attachment;
        rendering_info.pStencilAttachment = &stencil_attachment;
        m_command_list->beginRendering(&rendering_info);
    } else {
        const auto& render_pass_desc = render_pass->As<RenderPassBase>().GetDesc();
        assert(m_state->GetPipelineType() == PipelineType::kGraphics);
        const auto& vk_graphics_pipeline = m_state->As<VKGraphicsPipeline>();
        VKFramebuffer vk_framebuffer(m_device, framebuffer_desc, vk_graphics_pipeline.GetRenderPass());
        m_framebuffers.push_back(vk_framebuffer.TakeFramebuffer());

        vk::RenderPassBeginInfo render_pass_info = {};
        render_pass_info.renderPass = vk_graphics_pipeline.GetRenderPass();
        render_pass_info.framebuffer = m_framebuffers.back().get();
        render_pass_info.renderArea.extent.width = framebuffer_desc.width;
        render_pass_info.renderArea.extent.height = framebuffer_desc.height;
        std::vector<vk::ClearValue> clear_values;
        for (const auto& color : clear_desc.colors) {
            auto& clear_value = clear_values.emplace_back();
            clear_value.color.float32[0] = color.r;
            clear_value.color.float32[1] = color.g;
            clear_value.color.float32[2] = color.b;
            clear_value.color.float32[3] = color.a;
        }
        clear_values.resize(render_pass_desc.colors.size());
        if (render_pass_desc.depth_stencil.format != gli::FORMAT_UNDEFINED) {
            vk::ClearValue clear_value = {};
            clear_value.depthStencil.depth = clear_desc.depth;
            clear_value.depthStencil.stencil = clear_desc.stencil;
            clear_values.emplace_back(clear_value);
        }
        render_pass_info.clearValueCount = clear_values.size();
        render_pass_info.pClearValues = clear_values.data();
        vk::RenderPassAttachmentBeginInfo render_pass_attachment_begin_info = {};
        render_pass_attachment_begin_info.attachmentCount = vk_framebuffer.GetAttachments().size();
        render_pass_attachment_begin_info.pAttachments = vk_framebuffer.GetAttachments().data();
        render_pass_info.pNext = &render_pass_attachment_begin_info;
        m_command_list->beginRenderPass(render_pass_info, vk::SubpassContents::eInline);
    }
}

void VKCommandList::EndRenderPass()
{
    if (m_device.IsDynamicRenderingSupported()) {
        m_command_list->endRendering();
    } else {
        m_command_list->endRenderPass();
    }
}

void VKCommandList::BeginEvent(const std::string& name)
{
    if (m_device.GetAdapter().GetInstance().IsDebugUtilsSupported()) {
        vk::DebugUtilsLabelEXT label = {};
        label.pLabelName = name.c_str();
        m_command_list->beginDebugUtilsLabelEXT(&label);
    }
}

void VKCommandList::EndEvent()
{
    if (m_device.GetAdapter().GetInstance().IsDebugUtilsSupported()) {
        m_command_list->endDebugUtilsLabelEXT();
    }
}

void VKCommandList::Draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
{
    m_command_list->draw(vertex_count, instance_count, first_vertex, first_instance);
}

void VKCommandList::DrawIndexed(uint32_t index_count,
                                uint32_t instance_count,
                                uint32_t first_index,
                                int32_t vertex_offset,
                                uint32_t first_instance)
{
    m_command_list->drawIndexed(index_count, instance_count, first_index, vertex_offset, first_instance);
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
        m_command_list->drawIndirectCount(vk_argument_buffer.GetBuffer(), argument_buffer_offset,
                                          vk_count_buffer.GetBuffer(), count_buffer_offset, max_draw_count, stride);
    } else {
        assert(count_buffer_offset == 0);
        m_command_list->drawIndirect(vk_argument_buffer.GetBuffer(), argument_buffer_offset, max_draw_count, stride);
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
        m_command_list->drawIndexedIndirectCount(vk_argument_buffer.GetBuffer(), argument_buffer_offset,
                                                 vk_count_buffer.GetBuffer(), count_buffer_offset, max_draw_count,
                                                 stride);
    } else {
        assert(count_buffer_offset == 0);
        m_command_list->drawIndexedIndirect(vk_argument_buffer.GetBuffer(), argument_buffer_offset, max_draw_count,
                                            stride);
    }
}

void VKCommandList::Dispatch(uint32_t thread_group_count_x,
                             uint32_t thread_group_count_y,
                             uint32_t thread_group_count_z)
{
    m_command_list->dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z);
}

void VKCommandList::DispatchIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset)
{
    decltype(auto) vk_argument_buffer = argument_buffer->As<VKResource>();
    m_command_list->dispatchIndirect(vk_argument_buffer.GetBuffer(), argument_buffer_offset);
}

void VKCommandList::DispatchMesh(uint32_t thread_group_count_x,
                                 uint32_t thread_group_count_y,
                                 uint32_t thread_group_count_z)
{
    m_command_list->drawMeshTasksEXT(thread_group_count_x, thread_group_count_y, thread_group_count_z);
}

void VKCommandList::DispatchRays(const RayTracingShaderTables& shader_tables,
                                 uint32_t width,
                                 uint32_t height,
                                 uint32_t depth)
{
    m_command_list->traceRaysKHR(GetStridedDeviceAddressRegion(m_device, shader_tables.raygen),
                                 GetStridedDeviceAddressRegion(m_device, shader_tables.miss),
                                 GetStridedDeviceAddressRegion(m_device, shader_tables.hit),
                                 GetStridedDeviceAddressRegion(m_device, shader_tables.callable), width, height, depth);
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
        range.aspectMask = m_device.GetAspectFlags(static_cast<vk::Format>(vk_resource.GetFormat()));
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
        m_command_list->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                                        vk::PipelineStageFlagBits::eAllCommands, vk::DependencyFlagBits::eByRegion, 0,
                                        nullptr, 0, nullptr, image_memory_barriers.size(),
                                        image_memory_barriers.data());
    }
}

void VKCommandList::UAVResourceBarrier(const std::shared_ptr<Resource>& /*resource*/)
{
    vk::MemoryBarrier memory_barrier = {};
    memory_barrier.srcAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteKHR |
                                   vk::AccessFlagBits::eAccelerationStructureReadKHR |
                                   vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead;
    memory_barrier.dstAccessMask = memory_barrier.srcAccessMask;
    m_command_list->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands,
                                    vk::DependencyFlagBits::eByRegion, 1, &memory_barrier, 0, nullptr, 0, nullptr);
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

void VKCommandList::SetScissorRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
{
    vk::Rect2D rect = {};
    rect.offset.x = left;
    rect.offset.y = top;
    rect.extent.width = right - left;
    rect.extent.height = bottom - top;
    m_command_list->setScissor(0, 1, &rect);
}

void VKCommandList::IASetIndexBuffer(const std::shared_ptr<Resource>& resource, uint64_t offset, gli::format format)
{
    decltype(auto) vk_resource = resource->As<VKResource>();
    vk::IndexType index_type = GetVkIndexType(format);
    m_command_list->bindIndexBuffer(vk_resource.GetBuffer(), offset, index_type);
}

void VKCommandList::IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource, uint64_t offset)
{
    decltype(auto) vk_resource = resource->As<VKResource>();
    vk::Buffer buffers[] = { vk_resource.GetBuffer() };
    vk::DeviceSize offsets[] = { offset };
    m_command_list->bindVertexBuffers(slot, 1, buffers, offsets);
}

void VKCommandList::RSSetShadingRate(ShadingRate shading_rate, const std::array<ShadingRateCombiner, 2>& combiners)
{
    vk::Extent2D fragment_size = ConvertShadingRate(shading_rate);
    std::array<vk::FragmentShadingRateCombinerOpKHR, 2> vk_combiners = ConvertShadingRateCombiners(combiners);
    m_command_list->setFragmentShadingRateKHR(&fragment_size, vk_combiners.data());
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
        geometry_descs.emplace_back(m_device.FillRaytracingGeometryTriangles(desc.vertex, desc.index, desc.flags));
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
    infos.scratchData = m_device.GetDevice().getBufferAddress({ vk_scratch.GetBuffer() }) + scratch_offset;
    infos.pGeometries = geometry_descs.data();
    infos.geometryCount = geometry_descs.size();

    m_command_list->buildAccelerationStructuresKHR(1, &infos, range_infos.data());
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
    instance_address = m_device.GetDevice().getBufferAddress(vk_instance_data.GetBuffer()) + instance_offset;
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
    infos.scratchData = m_device.GetDevice().getBufferAddress({ vk_scratch.GetBuffer() }) + scratch_offset;
    infos.pGeometries = &top_as_geometry;
    infos.geometryCount = 1;

    m_command_list->buildAccelerationStructuresKHR(1, &infos, offset_infos.data());
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
    m_command_list->copyAccelerationStructureKHR(info);
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
    m_command_list->copyBuffer(vk_src_buffer.GetBuffer(), vk_dst_buffer.GetBuffer(), vk_regions);
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
    m_command_list->copyBufferToImage(vk_src_buffer.GetBuffer(), vk_dst_texture.GetImage(),
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
    m_command_list->copyImage(vk_src_texture.GetImage(), vk::ImageLayout::eTransferSrcOptimal,
                              vk_dst_texture.GetImage(), vk::ImageLayout::eTransferDstOptimal, vk_regions);
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
    m_command_list->resetQueryPool(vk_query_heap.GetQueryPool(), first_query, acceleration_structures.size());
    m_command_list->writeAccelerationStructuresPropertiesKHR(vk_acceleration_structures.size(),
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
    m_command_list->copyQueryPoolResults(vk_query_heap.GetQueryPool(), first_query, query_count,
                                         dst_buffer->As<VKResource>().GetBuffer(), dst_offset, sizeof(uint64_t),
                                         vk::QueryResultFlagBits::eWait);
}

void VKCommandList::SetName(const std::string& name)
{
    vk::DebugUtilsObjectNameInfoEXT info = {};
    info.pObjectName = name.c_str();
    info.objectType = vk::ObjectType::eCommandBuffer;
    info.objectHandle = reinterpret_cast<uint64_t>(static_cast<VkCommandBuffer>(m_command_list.get()));
    m_device.GetDevice().setDebugUtilsObjectNameEXT(info);
}

vk::CommandBuffer VKCommandList::GetCommandList()
{
    return m_command_list.get();
}
