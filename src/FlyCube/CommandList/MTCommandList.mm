#include "CommandList/MTCommandList.h"

#include "BindingSet/MTBindingSet.h"
#include "Device/MTDevice.h"
#include "Framebuffer/FramebufferBase.h"
#include "Pipeline/MTComputePipeline.h"
#include "Pipeline/MTGraphicsPipeline.h"
#include "Resource/MTResource.h"
#include "View/MTView.h"

namespace {

MTLIndexType ConvertIndexType(gli::format format)
{
    switch (format) {
    case gli::format::FORMAT_R16_UINT_PACK16:
        return MTLIndexTypeUInt16;
    case gli::format::FORMAT_R32_UINT_PACK32:
        return MTLIndexTypeUInt32;
    default:
        assert(false);
        return MTLIndexTypeUInt16;
    }
}

MTLLoadAction ConvertLoadAction(RenderPassLoadOp op)
{
    switch (op) {
    case RenderPassLoadOp::kLoad:
        return MTLLoadActionLoad;
    case RenderPassLoadOp::kClear:
        return MTLLoadActionClear;
    case RenderPassLoadOp::kDontCare:
        return MTLLoadActionDontCare;
    default:
        assert(false);
        return MTLLoadActionLoad;
    }
}

MTLStoreAction ConvertStoreAction(RenderPassStoreOp op)
{
    switch (op) {
    case RenderPassStoreOp::kStore:
        return MTLStoreActionStore;
    case RenderPassStoreOp::kDontCare:
        return MTLStoreActionDontCare;
    default:
        assert(false);
        return MTLStoreActionStore;
    }
}

MTLCullMode ConvertCullMode(CullMode cull_mode)
{
    switch (cull_mode) {
    case CullMode::kNone:
        return MTLCullModeNone;
    case CullMode::kFront:
        return MTLCullModeFront;
    case CullMode::kBack:
        return MTLCullModeBack;
    default:
        assert(false);
        return MTLCullModeNone;
    }
}

} // namespace

MTCommandList::MTCommandList(MTDevice& device, CommandListType type)
    : m_device(device)
{
    decltype(auto) command_queue = m_device.GetMTCommandQueue();
    m_command_buffer = [command_queue commandBuffer];
}

void MTCommandList::Reset()
{
    Close();
    decltype(auto) command_queue = m_device.GetMTCommandQueue();
    m_command_buffer = [command_queue commandBuffer];
    m_recorded_cmds = {};
    m_executed = false;
}

void MTCommandList::Close()
{
    m_render_encoder = nullptr;
    m_index_buffer.reset();
    m_index_format = gli::FORMAT_UNDEFINED;
    m_viewport = {};
    m_vertices.clear();
    m_state.reset();
    m_last_state.reset();
    m_binding_set.reset();
    m_last_binding_set.reset();
}

void MTCommandList::BindPipeline(const std::shared_ptr<Pipeline>& state)
{
    m_state = state;
}

void MTCommandList::BindBindingSet(const std::shared_ptr<BindingSet>& binding_set)
{
    m_binding_set = std::static_pointer_cast<MTBindingSet>(binding_set);
}

void MTCommandList::BeginRenderPass(const std::shared_ptr<RenderPass>& render_pass,
                                    const std::shared_ptr<Framebuffer>& framebuffer,
                                    const ClearDesc& clear_desc)
{
    MTLRenderPassDescriptor* render_pass_descriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    const RenderPassDesc& render_pass_desc = render_pass->GetDesc();
    const FramebufferDesc& framebuffer_desc = framebuffer->As<FramebufferBase>().GetDesc();

    bool has_attachment = false;

    auto add_attachment = [&](auto& attachment, gli::format format, RenderPassLoadOp load_op,
                              RenderPassStoreOp store_op, const std::shared_ptr<View>& view) {
        if (format == gli::format::FORMAT_UNDEFINED || !view) {
            return;
        }
        attachment.loadAction = ConvertLoadAction(load_op);
        attachment.storeAction = ConvertStoreAction(store_op);

        decltype(auto) mt_view = view->As<MTView>();
        attachment.level = mt_view.GetBaseMipLevel();
        attachment.slice = mt_view.GetBaseArrayLayer();
        attachment.texture = mt_view.GetTexture();
        if (!attachment.texture) {
            return;
        }

        if ([m_device.GetDevice() supportsFamily:MTLGPUFamilyApple5] ||
            [m_device.GetDevice() supportsFamily:MTLGPUFamilyCommon3]) {
            render_pass_descriptor.renderTargetArrayLength =
                std::max<uint32_t>(render_pass_descriptor.renderTargetArrayLength, view->GetLayerCount());
        }
        has_attachment = true;
    };

    for (size_t i = 0; i < render_pass_desc.colors.size(); ++i) {
        decltype(auto) color_attachment = render_pass_descriptor.colorAttachments[i];
        decltype(auto) color_desc = render_pass_desc.colors[i];
        add_attachment(color_attachment, color_desc.format, color_desc.load_op, color_desc.store_op,
                       framebuffer_desc.colors[i]);
    }
    for (size_t i = 0; i < clear_desc.colors.size(); ++i) {
        decltype(auto) color_attachment = render_pass_descriptor.colorAttachments[i];
        color_attachment.clearColor = MTLClearColorMake(clear_desc.colors[i].r, clear_desc.colors[i].g,
                                                        clear_desc.colors[i].b, clear_desc.colors[i].a);
    }

    decltype(auto) depth_stencil_desc = render_pass_desc.depth_stencil;
    if (depth_stencil_desc.format != gli::format::FORMAT_UNDEFINED) {
        decltype(auto) depth_attachment = render_pass_descriptor.depthAttachment;
        if (gli::is_depth(depth_stencil_desc.format)) {
            add_attachment(depth_attachment, depth_stencil_desc.format, depth_stencil_desc.depth_load_op,
                           depth_stencil_desc.depth_store_op, framebuffer_desc.depth_stencil);
        }
        depth_attachment.clearDepth = clear_desc.depth;

        decltype(auto) stencil_attachment = render_pass_descriptor.stencilAttachment;
        if (gli::is_stencil(depth_stencil_desc.format)) {
            add_attachment(stencil_attachment, depth_stencil_desc.format, depth_stencil_desc.stencil_load_op,
                           depth_stencil_desc.stencil_store_op, framebuffer_desc.depth_stencil);
        }
        stencil_attachment.clearStencil = clear_desc.stencil;
    }

    if (!has_attachment) {
        decltype(auto) dummy_texture = framebuffer->As<FramebufferBase>().GetDummyAttachment();
        if (!dummy_texture) {
            dummy_texture = m_device.CreateTexture(TextureType::k2D, BindFlag::kRenderTarget,
                                                   gli::format::FORMAT_R8_UNORM_PACK8, render_pass_desc.sample_count,
                                                   framebuffer_desc.width, framebuffer_desc.height, 1, 1);
            dummy_texture->CommitMemory(MemoryType::kDefault);
        }
        decltype(auto) color_attachment = render_pass_descriptor.colorAttachments[0];
        color_attachment.texture = dummy_texture->As<MTResource>().texture.res;
    }
    render_pass_descriptor.renderTargetWidth = framebuffer_desc.width;
    render_pass_descriptor.renderTargetHeight = framebuffer_desc.height;

    ApplyAndRecord([&render_encoder = m_render_encoder, &command_buffer = m_command_buffer, render_pass_descriptor] {
        render_encoder = [command_buffer renderCommandEncoderWithDescriptor:render_pass_descriptor];
        if (render_encoder == nullptr) {
            NSLog(@"Error: failed to create render pass");
        }
    });

    ApplyAndRecord(
        [&render_encoder = m_render_encoder, viewport = m_viewport] { [render_encoder setViewport:viewport]; });

    for (const auto& vertex : m_vertices) {
        ApplyAndRecord([&render_encoder = m_render_encoder, vertex] {
            [render_encoder setVertexBuffer:vertex.second offset:0 atIndex:vertex.first];
        });
    }
}

void MTCommandList::EndRenderPass()
{
    ApplyAndRecord([&render_encoder = m_render_encoder] {
        [render_encoder endEncoding];
        render_encoder = nullptr;
    });
    m_last_state.reset();
    m_last_binding_set.reset();
}

void MTCommandList::BeginEvent(const std::string& name) {}

void MTCommandList::EndEvent() {}

void MTCommandList::Draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
{
    ApplyState();
    ApplyAndRecord([&render_encoder = m_render_encoder, vertex_count, instance_count, first_vertex, first_instance] {
        if (first_instance > 0) {
            [render_encoder drawPrimitives:MTLPrimitiveTypeTriangle
                               vertexStart:first_vertex
                               vertexCount:vertex_count
                             instanceCount:instance_count
                              baseInstance:first_instance];
        } else {
            [render_encoder drawPrimitives:MTLPrimitiveTypeTriangle
                               vertexStart:first_vertex
                               vertexCount:vertex_count
                             instanceCount:instance_count];
        }
    });
}

void MTCommandList::DrawIndexed(uint32_t index_count,
                                uint32_t instance_count,
                                uint32_t first_index,
                                int32_t vertex_offset,
                                uint32_t first_instance)
{
    ApplyState();
    assert(m_index_buffer);
    decltype(auto) index_buffer = m_index_buffer->As<MTResource>().buffer.res;
    MTLIndexType index_format = ConvertIndexType(m_index_format);
    ApplyAndRecord([&render_encoder = m_render_encoder, index_buffer, index_count, index_format, instance_count,
                    first_index, vertex_offset, first_instance] {
        const uint32_t index_stride = index_format == MTLIndexTypeUInt32 ? 4 : 2;
        if (vertex_offset != 0 || first_instance > 0) {
            [render_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                       indexCount:index_count
                                        indexType:index_format
                                      indexBuffer:index_buffer
                                indexBufferOffset:index_stride * first_index
                                    instanceCount:instance_count
                                       baseVertex:vertex_offset
                                     baseInstance:first_instance];
        } else {
            [render_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                       indexCount:index_count
                                        indexType:index_format
                                      indexBuffer:index_buffer
                                indexBufferOffset:index_stride * first_index
                                    instanceCount:instance_count];
        }
    });
}

void MTCommandList::DrawIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset)
{
    ApplyState();
    decltype(auto) mt_argument_buffer = argument_buffer->As<MTResource>().buffer.res;
    ApplyAndRecord([&render_encoder = m_render_encoder, mt_argument_buffer, argument_buffer_offset] {
        [render_encoder drawPrimitives:MTLPrimitiveTypeTriangle
                        indirectBuffer:mt_argument_buffer
                  indirectBufferOffset:argument_buffer_offset];
    });
}

void MTCommandList::DrawIndexedIndirect(const std::shared_ptr<Resource>& argument_buffer,
                                        uint64_t argument_buffer_offset)
{
    ApplyState();
    decltype(auto) mt_argument_buffer = argument_buffer->As<MTResource>().buffer.res;
    assert(m_index_buffer);
    decltype(auto) index_buffer = m_index_buffer->As<MTResource>().buffer.res;
    MTLIndexType index_format = ConvertIndexType(m_index_format);
    ApplyAndRecord(
        [&render_encoder = m_render_encoder, mt_argument_buffer, argument_buffer_offset, index_format, index_buffer] {
            [render_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                        indexType:index_format
                                      indexBuffer:index_buffer
                                indexBufferOffset:0
                                   indirectBuffer:mt_argument_buffer
                             indirectBufferOffset:argument_buffer_offset];
        });
}

void MTCommandList::DrawIndirectCount(const std::shared_ptr<Resource>& argument_buffer,
                                      uint64_t argument_buffer_offset,
                                      const std::shared_ptr<Resource>& count_buffer,
                                      uint64_t count_buffer_offset,
                                      uint32_t max_draw_count,
                                      uint32_t stride)
{
    assert(false);
}

void MTCommandList::DrawIndexedIndirectCount(const std::shared_ptr<Resource>& argument_buffer,
                                             uint64_t argument_buffer_offset,
                                             const std::shared_ptr<Resource>& count_buffer,
                                             uint64_t count_buffer_offset,
                                             uint32_t max_draw_count,
                                             uint32_t stride)
{
    assert(false);
}

void MTCommandList::Dispatch(uint32_t thread_group_count_x,
                             uint32_t thread_group_count_y,
                             uint32_t thread_group_count_z)
{
    ApplyAndRecord([&command_buffer = m_command_buffer, thread_group_count_x, thread_group_count_y,
                    thread_group_count_z, binding_set = m_binding_set, state = m_state] {
        id<MTLComputeCommandEncoder> compute_encoder = [command_buffer computeCommandEncoder];
        if (binding_set) {
            binding_set->Apply(compute_encoder, state);
        }
        decltype(auto) mt_state = state->As<MTComputePipeline>();
        decltype(auto) mt_pipeline = mt_state.GetPipeline();
        [compute_encoder setComputePipelineState:mt_pipeline];
        MTLSize threadgroups_per_grid = { thread_group_count_x, thread_group_count_y, thread_group_count_z };
        [compute_encoder dispatchThreadgroups:threadgroups_per_grid threadsPerThreadgroup:mt_state.GetNumthreads()];
        [compute_encoder endEncoding];
    });
}

void MTCommandList::DispatchIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset)
{
    decltype(auto) mt_argument_buffer = argument_buffer->As<MTResource>().buffer.res;
    ApplyAndRecord([&command_buffer = m_command_buffer, mt_argument_buffer, argument_buffer_offset,
                    binding_set = m_binding_set, state = m_state] {
        id<MTLComputeCommandEncoder> compute_encoder = [command_buffer computeCommandEncoder];
        if (binding_set) {
            binding_set->Apply(compute_encoder, state);
        }
        decltype(auto) mt_state = state->As<MTComputePipeline>();
        decltype(auto) mt_pipeline = mt_state.GetPipeline();
        [compute_encoder setComputePipelineState:mt_pipeline];
        [compute_encoder dispatchThreadgroupsWithIndirectBuffer:mt_argument_buffer
                                           indirectBufferOffset:argument_buffer_offset
                                          threadsPerThreadgroup:mt_state.GetNumthreads()];
        [compute_encoder endEncoding];
    });
}

void MTCommandList::DispatchMesh(uint32_t thread_group_count_x)
{
    assert(false);
}

void MTCommandList::DispatchRays(const RayTracingShaderTables& shader_tables,
                                 uint32_t width,
                                 uint32_t height,
                                 uint32_t depth)
{
    assert(false);
}

void MTCommandList::ResourceBarrier(const std::vector<ResourceBarrierDesc>& barriers) {}

void MTCommandList::UAVResourceBarrier(const std::shared_ptr<Resource>& /*resource*/) {}

void MTCommandList::SetViewport(float x, float y, float width, float height)
{
    m_viewport.originX = x;
    m_viewport.originY = y;
    m_viewport.width = width;
    m_viewport.height = height;
    m_viewport.znear = 0.0;
    m_viewport.zfar = 1.0;

    if (!m_render_encoder) {
        return;
    }

    ApplyAndRecord(
        [&render_encoder = m_render_encoder, viewport = m_viewport] { [render_encoder setViewport:viewport]; });
}

void MTCommandList::SetScissorRect(int32_t left, int32_t top, uint32_t right, uint32_t bottom) {}

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

    if (!m_render_encoder) {
        return;
    }

    ApplyAndRecord([&render_encoder = m_render_encoder, vertex, index] {
        [render_encoder setVertexBuffer:vertex offset:0 atIndex:index];
    });
}

void MTCommandList::RSSetShadingRate(ShadingRate shading_rate, const std::array<ShadingRateCombiner, 2>& combiners)
{
    assert(false);
}

void MTCommandList::BuildBottomLevelAS(const std::shared_ptr<Resource>& src,
                                       const std::shared_ptr<Resource>& dst,
                                       const std::shared_ptr<Resource>& scratch,
                                       uint64_t scratch_offset,
                                       const std::vector<RaytracingGeometryDesc>& descs,
                                       BuildAccelerationStructureFlags flags)
{
    NSMutableArray* geometry_descs = [NSMutableArray array];
    for (const auto& desc : descs) {
        MTLAccelerationStructureTriangleGeometryDescriptor* geometry_desc =
            FillRaytracingGeometryDesc(desc.vertex, desc.index, desc.flags);
        [geometry_descs addObject:geometry_desc];
    }

    MTLPrimitiveAccelerationStructureDescriptor* acceleration_structure_desc =
        [MTLPrimitiveAccelerationStructureDescriptor descriptor];
    acceleration_structure_desc.geometryDescriptors = geometry_descs;

    decltype(auto) mt_dst = dst->As<MTResource>();
    decltype(auto) mt_scratch = scratch->As<MTResource>();

    ApplyAndRecord(
        [&command_buffer = m_command_buffer, mt_dst, mt_scratch, scratch_offset, acceleration_structure_desc] {
            id<MTLAccelerationStructureCommandEncoder> command_encoder =
                [command_buffer accelerationStructureCommandEncoder];
            [command_encoder buildAccelerationStructure:mt_dst.acceleration_structure
                                             descriptor:acceleration_structure_desc
                                          scratchBuffer:mt_scratch.buffer.res
                                    scratchBufferOffset:scratch_offset];
            [command_encoder endEncoding];
        });
}

// TODO: patch on GPU
id<MTLBuffer> MTCommandList::PatchInstanceData(const std::shared_ptr<Resource>& instance_data,
                                               uint64_t instance_offset,
                                               uint32_t instance_count)
{
    MTLResourceOptions buffer_options = MTLStorageModeShared << MTLResourceStorageModeShift;
    NSUInteger buffer_size = instance_count * sizeof(MTLIndirectAccelerationStructureInstanceDescriptor);
    id<MTLBuffer> buffer = [m_device.GetDevice() newBufferWithLength:buffer_size options:buffer_options];

    uint8_t* instance_ptr =
        static_cast<uint8_t*>(instance_data->As<MTResource>().buffer.res.contents) + instance_offset;
    uint8_t* patched_instance_ptr = static_cast<uint8_t*>(buffer.contents);
    for (uint32_t i = 0; i < instance_count; ++i) {
        RaytracingGeometryInstance& instance = reinterpret_cast<RaytracingGeometryInstance*>(instance_ptr)[i];
        MTLIndirectAccelerationStructureInstanceDescriptor& patched_instance =
            reinterpret_cast<MTLIndirectAccelerationStructureInstanceDescriptor*>(patched_instance_ptr)[i];

        auto matrix = glm::mat4x3(instance.transform);
        memcpy(&patched_instance.transformationMatrix, &matrix, sizeof(patched_instance.transformationMatrix));
        patched_instance.mask = instance.instance_mask;
        patched_instance.accelerationStructureID = { instance.acceleration_structure_handle };
    }

    return buffer;
}

void MTCommandList::BuildTopLevelAS(const std::shared_ptr<Resource>& src,
                                    const std::shared_ptr<Resource>& dst,
                                    const std::shared_ptr<Resource>& scratch,
                                    uint64_t scratch_offset,
                                    const std::shared_ptr<Resource>& instance_data,
                                    uint64_t instance_offset,
                                    uint32_t instance_count,
                                    BuildAccelerationStructureFlags flags)
{
    MTLInstanceAccelerationStructureDescriptor* acceleration_structure_desc =
        [MTLInstanceAccelerationStructureDescriptor descriptor];
    acceleration_structure_desc.instanceCount = instance_count;
    acceleration_structure_desc.instanceDescriptorBuffer =
        PatchInstanceData(instance_data, instance_offset, instance_count);
    acceleration_structure_desc.instanceDescriptorBufferOffset = 0;
    acceleration_structure_desc.instanceDescriptorType = MTLAccelerationStructureInstanceDescriptorTypeIndirect;

    decltype(auto) mt_dst = dst->As<MTResource>();
    decltype(auto) mt_scratch = scratch->As<MTResource>();

    ApplyAndRecord(
        [&command_buffer = m_command_buffer, mt_dst, mt_scratch, scratch_offset, acceleration_structure_desc] {
            id<MTLAccelerationStructureCommandEncoder> command_encoder =
                [command_buffer accelerationStructureCommandEncoder];
            [command_encoder buildAccelerationStructure:mt_dst.acceleration_structure
                                             descriptor:acceleration_structure_desc
                                          scratchBuffer:mt_scratch.buffer.res
                                    scratchBufferOffset:scratch_offset];
            [command_encoder endEncoding];
        });
}

void MTCommandList::CopyAccelerationStructure(const std::shared_ptr<Resource>& src,
                                              const std::shared_ptr<Resource>& dst,
                                              CopyAccelerationStructureMode mode)
{
    assert(false);
}

void MTCommandList::CopyBuffer(const std::shared_ptr<Resource>& src_buffer,
                               const std::shared_ptr<Resource>& dst_buffer,
                               const std::vector<BufferCopyRegion>& regions)
{
    ApplyAndRecord([&command_buffer = m_command_buffer, src_buffer, dst_buffer, regions] {
        id<MTLBlitCommandEncoder> blit_encoder = [command_buffer blitCommandEncoder];
        decltype(auto) mt_src_buffer = src_buffer->As<MTResource>();
        decltype(auto) mt_dst_buffer = dst_buffer->As<MTResource>();
        for (const auto& region : regions) {
            [blit_encoder copyFromBuffer:mt_src_buffer.buffer.res
                            sourceOffset:region.src_offset
                                toBuffer:mt_dst_buffer.buffer.res
                       destinationOffset:region.dst_offset
                                    size:region.num_bytes];
        }
        [blit_encoder endEncoding];
    });
}

void MTCommandList::CopyBufferToTexture(const std::shared_ptr<Resource>& src_buffer,
                                        const std::shared_ptr<Resource>& dst_texture,
                                        const std::vector<BufferToTextureCopyRegion>& regions)
{
    ApplyAndRecord([&command_buffer = m_command_buffer, src_buffer, dst_texture, regions] {
        id<MTLBlitCommandEncoder> blit_encoder = [command_buffer blitCommandEncoder];
        decltype(auto) mt_src_buffer = src_buffer->As<MTResource>();
        decltype(auto) mt_dst_texture = dst_texture->As<MTResource>();
        auto format = dst_texture->GetFormat();
        for (const auto& region : regions) {
            uint32_t bytes_per_image = 0;
            if (gli::is_compressed(format)) {
                auto extent = gli::block_extent(format);
                bytes_per_image =
                    region.buffer_row_pitch *
                    ((region.texture_extent.height + gli::block_extent(format).y - 1) / gli::block_extent(format).y);
            } else {
                bytes_per_image = region.buffer_row_pitch * region.texture_extent.height;
            }
            MTLSize region_size = { region.texture_extent.width, region.texture_extent.height,
                                    region.texture_extent.depth };
            [blit_encoder copyFromBuffer:mt_src_buffer.buffer.res
                            sourceOffset:region.buffer_offset
                       sourceBytesPerRow:region.buffer_row_pitch
                     sourceBytesPerImage:bytes_per_image
                              sourceSize:region_size
                               toTexture:mt_dst_texture.texture.res
                        destinationSlice:region.texture_array_layer
                        destinationLevel:region.texture_mip_level
                       destinationOrigin:{ (uint32_t)region.texture_offset.x, (uint32_t)region.texture_offset.y,
                                           (uint32_t)region.texture_offset.z }];
        }
        [blit_encoder endEncoding];
    });
}

void MTCommandList::CopyTexture(const std::shared_ptr<Resource>& src_texture,
                                const std::shared_ptr<Resource>& dst_texture,
                                const std::vector<TextureCopyRegion>& regions)
{
    ApplyAndRecord([&command_buffer = m_command_buffer, src_texture, dst_texture, regions] {
        id<MTLBlitCommandEncoder> blit_encoder = [command_buffer blitCommandEncoder];
        decltype(auto) mt_src_texture = src_texture->As<MTResource>();
        decltype(auto) mt_dst_texture = dst_texture->As<MTResource>();
        auto format = dst_texture->GetFormat();
        for (const auto& region : regions) {
            MTLSize region_size = { region.extent.width, region.extent.height, region.extent.depth };
            MTLOrigin src_origin = { (uint32_t)region.src_offset.x, (uint32_t)region.src_offset.y,
                                     (uint32_t)region.src_offset.z };
            MTLOrigin dst_origin = { (uint32_t)region.dst_offset.x, (uint32_t)region.dst_offset.y,
                                     (uint32_t)region.dst_offset.z };
            [blit_encoder copyFromTexture:mt_src_texture.texture.res
                              sourceSlice:region.src_array_layer
                              sourceLevel:region.src_mip_level
                             sourceOrigin:src_origin
                               sourceSize:region_size
                                toTexture:mt_dst_texture.texture.res
                         destinationSlice:region.dst_array_layer
                         destinationLevel:region.dst_mip_level
                        destinationOrigin:dst_origin];
        }
        [blit_encoder endEncoding];
    });
}

void MTCommandList::WriteAccelerationStructuresProperties(
    const std::vector<std::shared_ptr<Resource>>& acceleration_structures,
    const std::shared_ptr<QueryHeap>& query_heap,
    uint32_t first_query)
{
    assert(false);
}

void MTCommandList::ResolveQueryData(const std::shared_ptr<QueryHeap>& query_heap,
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
    if (m_executed) {
        decltype(auto) command_queue = m_device.GetMTCommandQueue();
        m_command_buffer = [command_queue commandBuffer];
        for (const auto& cmd : m_recorded_cmds) {
            cmd();
        }
    }
    m_executed = true;
}

void MTCommandList::ApplyBindingSet()
{
    if (!m_last_binding_set.expired() && m_last_binding_set.lock() == m_binding_set) {
        return;
    }

    assert(m_render_encoder);
    assert(m_state->GetPipelineType() == PipelineType::kGraphics);

    ApplyAndRecord([&render_encoder = m_render_encoder, binding_set = m_binding_set, state = m_state] {
        binding_set->Apply(render_encoder, state);
    });

    m_last_binding_set = m_binding_set;
}

void MTCommandList::ApplyState()
{
    ApplyBindingSet();
    if (!m_last_state.expired() && m_last_state.lock() == m_state) {
        return;
    }

    assert(m_render_encoder);
    assert(m_state->GetPipelineType() == PipelineType::kGraphics);

    decltype(auto) mt_state = m_state->As<MTGraphicsPipeline>();
    decltype(auto) mt_pipeline = mt_state.GetPipeline();
    decltype(auto) mt_depth_stencil = mt_state.GetDepthStencil();
    decltype(auto) rasterizer_desc = mt_state.GetDesc().rasterizer_desc;
    int32_t depth_bias = rasterizer_desc.depth_bias;
    MTLCullMode cull_mode = ConvertCullMode(rasterizer_desc.cull_mode);

    ApplyAndRecord([&render_encoder = m_render_encoder, mt_pipeline, mt_depth_stencil, depth_bias, cull_mode] {
        [render_encoder setRenderPipelineState:mt_pipeline];
        [render_encoder setDepthStencilState:mt_depth_stencil];
        [render_encoder setDepthBias:depth_bias slopeScale:0 clamp:0];
        [render_encoder setCullMode:cull_mode];
    });

    m_last_state = m_state;
}

void MTCommandList::ApplyAndRecord(std::function<void()>&& cmd)
{
    cmd();
    m_recorded_cmds.push_back(std::move(cmd));
}
