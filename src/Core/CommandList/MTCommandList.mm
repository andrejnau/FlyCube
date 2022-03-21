#include "CommandList/MTCommandList.h"
#include <Device/MTDevice.h>
#include <View/MTView.h>
#include <Resource/MTResource.h>
#include <Framebuffer/FramebufferBase.h>
#include <Pipeline/MTGraphicsPipeline.h>
#include <BindingSet/MTBindingSet.h>

MTCommandList::MTCommandList(MTDevice& device, CommandListType type)
    : m_device(device)
{
    decltype(auto) command_queue = device.GetMTCommandQueue();
    m_command_buffer = [command_queue commandBuffer];
}

void MTCommandList::Reset()
{
    decltype(auto) command_queue = m_device.GetMTCommandQueue();
    m_command_buffer = [command_queue commandBuffer];
    m_render_encoder = nullptr;
    m_index_buffer.reset();
    m_index_format = gli::FORMAT_UNDEFINED;
    m_viewport = {};
    m_vertices.clear();
    m_state.reset();
    m_binding_set.reset();
    m_recorded_cmds = {};
    m_executed = false;
}

void MTCommandList::Close()
{
}

void MTCommandList::BindPipeline(const std::shared_ptr<Pipeline>& state)
{
    if (state == m_state)
        return;
    m_state = std::static_pointer_cast<MTGraphicsPipeline>(state);
    if (!m_render_encoder)
        return;

    decltype(auto) mt_pipeline = m_state->GetPipeline();
    decltype(auto) mt_depth_stencil = m_state->GetDepthStencil();
    ApplyAndRecord([&render_encoder = m_render_encoder, mt_pipeline, mt_depth_stencil] {
        [render_encoder setRenderPipelineState:mt_pipeline];
        [render_encoder setDepthStencilState:mt_depth_stencil];
    });
}

void MTCommandList::BindBindingSet(const std::shared_ptr<BindingSet>& binding_set)
{
    if (binding_set == m_binding_set)
        return;
    m_binding_set = std::static_pointer_cast<MTBindingSet>(binding_set);
    if (!m_render_encoder)
        return;
    
    ApplyAndRecord([&render_encoder = m_render_encoder, binding_set = m_binding_set, state = m_state] {
        binding_set->Apply(render_encoder, state);
    });
}

static MTLLoadAction Convert(RenderPassLoadOp op)
{
    switch (op)
    {
    case RenderPassLoadOp::kLoad:
        return MTLLoadActionLoad;
    case RenderPassLoadOp::kClear:
        return MTLLoadActionClear;
    case RenderPassLoadOp::kDontCare:
        return MTLLoadActionDontCare;
    }
    assert(false);
    return MTLLoadActionLoad;
}

static MTLStoreAction Convert(RenderPassStoreOp op)
{
    switch (op)
    {
    case RenderPassStoreOp::kStore:
        return MTLStoreActionStore;
    case RenderPassStoreOp::kDontCare:
        return MTLStoreActionDontCare;
    }
    assert(false);
    return MTLStoreActionStore;
}

void MTCommandList::BeginRenderPass(const std::shared_ptr<RenderPass>& render_pass, const std::shared_ptr<Framebuffer>& framebuffer, const ClearDesc& clear_desc)
{
    MTLRenderPassDescriptor* render_pass_descriptor = [MTLRenderPassDescriptor new];
    const RenderPassDesc& render_pass_desc = render_pass->GetDesc();
    const FramebufferDesc& framebuffer_desc = framebuffer->As<FramebufferBase>().GetDesc();
    
    auto add_attachment = [&](auto& attachment, gli::format format, RenderPassLoadOp load_op, RenderPassStoreOp store_op, const std::shared_ptr<View>& view)
    {
        if (format == gli::format::FORMAT_UNDEFINED || !view)
            return;
        attachment.loadAction = Convert(load_op);
        attachment.storeAction = Convert(store_op);
        
        decltype(auto) mt_view = view->As<MTView>();
        attachment.level = mt_view.GetBaseMipLevel();
        attachment.slice = mt_view.GetBaseArrayLayer();
        decltype(auto) resource = mt_view.GetResource();
        if (!resource)
            return;
        decltype(auto) mt_resource = resource->As<MTResource>();
        attachment.texture = mt_resource.texture.res;
    };
    
    for (size_t i = 0; i < render_pass_desc.colors.size(); ++i)
    {
        decltype(auto) color_attachment = render_pass_descriptor.colorAttachments[i];
        decltype(auto) color_desc = render_pass_desc.colors[i];
        add_attachment(color_attachment, color_desc.format, color_desc.load_op, color_desc.store_op, framebuffer_desc.colors[i]);
    }
    for (size_t i = 0; i < clear_desc.colors.size(); ++i)
    {
        decltype(auto) color_attachment = render_pass_descriptor.colorAttachments[i];
        color_attachment.clearColor = MTLClearColorMake(clear_desc.colors[i].r,
                                                        clear_desc.colors[i].g,
                                                        clear_desc.colors[i].b,
                                                        clear_desc.colors[i].a);
    }
    
    decltype(auto) depth_stencil_desc = render_pass_desc.depth_stencil;
    if (depth_stencil_desc.format != gli::format::FORMAT_UNDEFINED)
    {
        decltype(auto) depth_attachment = render_pass_descriptor.depthAttachment;
        if (gli::is_depth(depth_stencil_desc.format))
        {
            add_attachment(depth_attachment, depth_stencil_desc.format, depth_stencil_desc.depth_load_op, depth_stencil_desc.depth_store_op, framebuffer_desc.depth_stencil);
        }
        depth_attachment.clearDepth = clear_desc.depth;

        decltype(auto) stencil_attachment = render_pass_descriptor.stencilAttachment;
        if (gli::is_stencil(depth_stencil_desc.format))
        {
            add_attachment(stencil_attachment, depth_stencil_desc.format, depth_stencil_desc.stencil_load_op, depth_stencil_desc.stencil_store_op, framebuffer_desc.depth_stencil);
        }
        stencil_attachment.clearStencil = clear_desc.stencil;
    }

    ApplyAndRecord([&render_encoder = m_render_encoder, &command_buffer = m_command_buffer, render_pass_descriptor,
                    viewport = m_viewport, state = m_state, vertices = m_vertices, binding_set = m_binding_set] {
        render_encoder = [command_buffer renderCommandEncoderWithDescriptor:render_pass_descriptor];
        [render_encoder setViewport:viewport];
        if (state)
        {
            [render_encoder setRenderPipelineState:state->GetPipeline()];
            [render_encoder setDepthStencilState:state->GetDepthStencil()];
        }
        for (const auto& vertex : vertices)
        {
            [render_encoder setVertexBuffer:vertex.second
                                       offset:0
                                      atIndex:vertex.first];
        }
        if (binding_set)
        {
            binding_set->Apply(render_encoder, state);
        }
    });
}

void MTCommandList::EndRenderPass()
{
    ApplyAndRecord([&render_encoder = m_render_encoder, &state = m_state] {
        [render_encoder endEncoding];
        render_encoder = nullptr;
    });
}

void MTCommandList::BeginEvent(const std::string& name)
{
}

void MTCommandList::EndEvent()
{
}

void MTCommandList::Draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
{
    assert(false);
}

static MTLIndexType GetIndexType(gli::format format)
{
    switch (format)
    {
    case gli::format::FORMAT_R16_UINT_PACK16:
        return MTLIndexTypeUInt16;
    case gli::format::FORMAT_R32_UINT_PACK32:
        return MTLIndexTypeUInt32;
    default:
        assert(false);
        return MTLIndexTypeUInt16;
    }
}

void MTCommandList::DrawIndexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance)
{
    assert(m_index_buffer);
    decltype(auto) index = m_index_buffer->As<MTResource>().buffer.res;
    MTLIndexType index_format = GetIndexType(m_index_format);
    ApplyAndRecord([&render_encoder = m_render_encoder, index_count, index_format, index] {
        [render_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                     indexCount:index_count
                                      indexType:index_format
                                    indexBuffer:index
                              indexBufferOffset:0
                                  instanceCount:1];
    });
}

void MTCommandList::DrawIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset)
{
    decltype(auto) mt_argument_buffer = argument_buffer->As<MTResource>().buffer.res;
    ApplyAndRecord([&render_encoder = m_render_encoder, mt_argument_buffer, argument_buffer_offset] {
        [render_encoder drawPrimitives:MTLPrimitiveTypeTriangle
                        indirectBuffer:mt_argument_buffer
                  indirectBufferOffset:argument_buffer_offset];
    });
}

void MTCommandList::DrawIndexedIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset)
{
    decltype(auto) mt_argument_buffer = argument_buffer->As<MTResource>().buffer.res;
    MTLIndexType index_format = GetIndexType(m_index_format);
    assert(m_index_buffer);
    decltype(auto) index = m_index_buffer->As<MTResource>().buffer.res;
    ApplyAndRecord([&render_encoder = m_render_encoder, mt_argument_buffer, argument_buffer_offset, index_format, index] {
        [render_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                    indexType:index_format
                                  indexBuffer:index
                            indexBufferOffset:0
                               indirectBuffer:mt_argument_buffer
                         indirectBufferOffset:argument_buffer_offset];
    });
}

void MTCommandList::DrawIndirectCount(
    const std::shared_ptr<Resource>& argument_buffer,
    uint64_t argument_buffer_offset,
    const std::shared_ptr<Resource>& count_buffer,
    uint64_t count_buffer_offset,
    uint32_t max_draw_count,
    uint32_t stride)
{
    assert(false);
}

void MTCommandList::DrawIndexedIndirectCount(
    const std::shared_ptr<Resource>& argument_buffer,
    uint64_t argument_buffer_offset,
    const std::shared_ptr<Resource>& count_buffer,
    uint64_t count_buffer_offset,
    uint32_t max_draw_count,
    uint32_t stride)
{
    assert(false);
}

void MTCommandList::Dispatch(uint32_t thread_group_count_x, uint32_t thread_group_count_y, uint32_t thread_group_count_z)
{
    assert(false);
}

void MTCommandList::DispatchIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset)
{
    assert(false);
}

void MTCommandList::DispatchMesh(uint32_t thread_group_count_x)
{
    assert(false);
}

void MTCommandList::DispatchRays(const RayTracingShaderTables& shader_tables, uint32_t width, uint32_t height, uint32_t depth)
{
    assert(false);
}

void MTCommandList::ResourceBarrier(const std::vector<ResourceBarrierDesc>& barriers)
{
}

void MTCommandList::UAVResourceBarrier(const std::shared_ptr<Resource>& /*resource*/)
{
}

void MTCommandList::SetViewport(float x, float y, float width, float height)
{
    m_viewport.originX = x;
    m_viewport.originY = y;
    m_viewport.width = width;
    m_viewport.height = height;
    m_viewport.znear = 0.0;
    m_viewport.zfar = 1.0;
    if (!m_render_encoder)
        return;

    ApplyAndRecord([&render_encoder = m_render_encoder, viewport = m_viewport] {
        [render_encoder setViewport:viewport];
    });
}

void MTCommandList::SetScissorRect(int32_t left, int32_t top, uint32_t right, uint32_t bottom)
{
}

void MTCommandList::IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format)
{
    m_index_buffer = resource;
    m_index_format = format;
}

void MTCommandList::IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource)
{
    decltype(auto) vertex = resource->As<MTResource>().buffer.res;
    uint32_t index = m_device.GetMaxPerStageBufferCount() - slot - 1;
    m_vertices[index] = vertex;
    if (!m_render_encoder)
        return;

    ApplyAndRecord([&render_encoder = m_render_encoder, vertex, index] {
        [render_encoder setVertexBuffer:vertex
                                   offset:0
                                  atIndex:index];
    });
}

void MTCommandList::RSSetShadingRate(ShadingRate shading_rate, const std::array<ShadingRateCombiner, 2>& combiners)
{
    assert(false);
}

void MTCommandList::BuildBottomLevelAS(
    const std::shared_ptr<Resource>& src,
    const std::shared_ptr<Resource>& dst,
    const std::shared_ptr<Resource>& scratch,
    uint64_t scratch_offset,
    const std::vector<RaytracingGeometryDesc>& descs,
    BuildAccelerationStructureFlags flags)
{
    assert(false);
}

void MTCommandList::BuildTopLevelAS(
    const std::shared_ptr<Resource>& src,
    const std::shared_ptr<Resource>& dst,
    const std::shared_ptr<Resource>& scratch,
    uint64_t scratch_offset,
    const std::shared_ptr<Resource>& instance_data,
    uint64_t instance_offset,
    uint32_t instance_count,
    BuildAccelerationStructureFlags flags)
{
    assert(false);
}

void MTCommandList::CopyAccelerationStructure(const std::shared_ptr<Resource>& src, const std::shared_ptr<Resource>& dst, CopyAccelerationStructureMode mode)
{
    assert(false);
}

void MTCommandList::CopyBuffer(const std::shared_ptr<Resource>& src_buffer, const std::shared_ptr<Resource>& dst_buffer,
                               const std::vector<BufferCopyRegion>& regions)
{
    ApplyAndRecord([&command_buffer = m_command_buffer, src_buffer, dst_buffer, regions] {
        id<MTLBlitCommandEncoder> blit_encoder = [command_buffer blitCommandEncoder];
        decltype(auto) mt_src_buffer = src_buffer->As<MTResource>();
        decltype(auto) mt_dst_buffer = dst_buffer->As<MTResource>();
        for (const auto& region : regions)
        {
            [blit_encoder copyFromBuffer:mt_src_buffer.buffer.res
                            sourceOffset:region.src_offset
                                toBuffer:mt_dst_buffer.buffer.res
                       destinationOffset:region.dst_offset
                                    size:region.num_bytes];
        }
        [blit_encoder endEncoding];
    });
}

void MTCommandList::CopyBufferToTexture(const std::shared_ptr<Resource>& src_buffer, const std::shared_ptr<Resource>& dst_texture,
                                        const std::vector<BufferToTextureCopyRegion>& regions)
{
    ApplyAndRecord([&command_buffer = m_command_buffer, src_buffer, dst_texture, regions] {
        id<MTLBlitCommandEncoder> blit_encoder = [command_buffer blitCommandEncoder];
        decltype(auto) mt_src_buffer = src_buffer->As<MTResource>();
        decltype(auto) mt_dst_texture = dst_texture->As<MTResource>();
        auto format = dst_texture->GetFormat();
        for (const auto& region : regions)
        {
            uint32_t bytes_per_image = 0;
            if (gli::is_compressed(format))
            {
                auto extent = gli::block_extent(format);
                bytes_per_image = region.buffer_row_pitch * ((region.texture_extent.height + gli::block_extent(format).y - 1) / gli::block_extent(format).y);
            }
            else
            {
                bytes_per_image = region.buffer_row_pitch * region.texture_extent.height;
            }
            MTLSize region_size = {region.texture_extent.width, region.texture_extent.height, region.texture_extent.depth};
            [blit_encoder copyFromBuffer:mt_src_buffer.buffer.res
                            sourceOffset:region.buffer_offset
                       sourceBytesPerRow:region.buffer_row_pitch
                     sourceBytesPerImage:bytes_per_image
                              sourceSize:region_size
                               toTexture:mt_dst_texture.texture.res
                        destinationSlice:region.texture_array_layer
                        destinationLevel:region.texture_mip_level
                       destinationOrigin:{(uint32_t)region.texture_offset.x, (uint32_t)region.texture_offset.y, (uint32_t)region.texture_offset.z}];
        }
        [blit_encoder endEncoding];
    });
}

void MTCommandList::CopyTexture(const std::shared_ptr<Resource>& src_texture, const std::shared_ptr<Resource>& dst_texture,
                                const std::vector<TextureCopyRegion>& regions)
{
    assert(false);
}

void MTCommandList::WriteAccelerationStructuresProperties(
    const std::vector<std::shared_ptr<Resource>>& acceleration_structures,
    const std::shared_ptr<QueryHeap>& query_heap,
    uint32_t first_query)
{
    assert(false);
}

void MTCommandList::ResolveQueryData(
    const std::shared_ptr<QueryHeap>& query_heap,
    uint32_t first_query,
    uint32_t query_count,
    const std::shared_ptr<Resource>& dst_buffer,
    uint64_t dst_offset)
{
    assert(false);
}

id<MTLCommandBuffer> MTCommandList::GetCommandBuffer()
{
    return m_command_buffer;
}

void MTCommandList::OnSubmit()
{
    if (m_executed)
    {
        decltype(auto) command_queue = m_device.GetMTCommandQueue();
        m_command_buffer = [command_queue commandBuffer];
        for (const auto& cmd : m_recorded_cmds)
        {
            cmd();
        }
    }
    m_executed = true;
}

void MTCommandList::ApplyAndRecord(std::function<void()>&& cmd)
{
    cmd();
    m_recorded_cmds.push_back(std::move(cmd));
}
