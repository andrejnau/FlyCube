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

id<MTL4ArgumentTable> CreateArgumentTable(MTDevice& device)
{
    MTL4ArgumentTableDescriptor* argument_table_descriptor = [MTL4ArgumentTableDescriptor new];
    argument_table_descriptor.initializeBindings = true;
    argument_table_descriptor.maxBufferBindCount = device.GetMaxPerStageBufferCount();
    argument_table_descriptor.maxSamplerStateBindCount = 16;
    argument_table_descriptor.maxTextureBindCount = 128;

    NSError* error = nullptr;
    auto argument_table = [device.GetDevice() newArgumentTableWithDescriptor:argument_table_descriptor error:&error];
    if (!argument_table) {
        NSLog(@"Error: failed to create MTL4ArgumentTable: %@", error);
    }
    return argument_table;
}

MTLStages ResourceStateToMTLStages(ResourceState state)
{
    MTLStages stages = 0;

    if (state &
        (ResourceState::kVertexAndConstantBuffer | ResourceState::kIndexBuffer | ResourceState::kUnorderedAccess |
         ResourceState::kNonPixelShaderResource | ResourceState::kIndirectArgument)) {
        stages |= MTLStageVertex | MTLStageObject | MTLStageMesh;
    }

    if (state &
        (ResourceState::kVertexAndConstantBuffer | ResourceState::kRenderTarget | ResourceState::kUnorderedAccess |
         ResourceState::kDepthStencilWrite | ResourceState::kDepthStencilRead | ResourceState::kPixelShaderResource |
         ResourceState::kIndirectArgument | ResourceState::kShadingRateSource)) {
        stages |= MTLStageFragment;
    }

    if (state & (ResourceState::kVertexAndConstantBuffer | ResourceState::kUnorderedAccess |
                 ResourceState::kNonPixelShaderResource | ResourceState::kIndirectArgument)) {
        stages |= MTLStageDispatch;
    }

    if (state & (ResourceState::kCopyDest | ResourceState::kCopySource)) {
        stages |= MTLStageBlit;
    }

    if (state & (ResourceState::kRaytracingAccelerationStructure)) {
        stages |= MTLStageAccelerationStructure;
    }

    return stages;
}

} // namespace

MTCommandList::MTCommandList(MTDevice& device, CommandListType type)
    : m_device(device)
{
    Reset();
}

void MTCommandList::Reset()
{
    Close();
    m_closed = false;

    m_command_buffer = [m_device.GetDevice() newCommandBuffer];
    if (!m_allocator) {
        m_allocator = [m_device.GetDevice() newCommandAllocator];
    } else {
        [m_allocator reset];
    }
    [m_command_buffer beginCommandBufferWithAllocator:m_allocator];

    CreateArgumentTables();
    m_residency_set = m_device.CreateResidencySet();
    [m_command_buffer useResidencySet:m_residency_set];

    static constexpr MTLStages kRenderStages = MTLStageVertex | MTLStageObject | MTLStageMesh | MTLStageFragment;
    static constexpr MTLStages kComputeStages = MTLStageDispatch | MTLStageBlit | MTLStageAccelerationStructure;

    m_render_barrier_after_stages = MTLStageAll;
    m_render_barrier_before_stages = kRenderStages;
    m_compute_barrier_after_stages = MTLStageAll;
    m_compute_barrier_before_stages = kComputeStages;
}

void MTCommandList::Close()
{
    if (m_closed) {
        return;
    }

    CloseComputeEncoder();

    [m_command_buffer endCommandBuffer];
    m_render_encoder = nullptr;
    m_index_buffer = nullptr;
    m_index_format = gli::FORMAT_UNDEFINED;
    m_viewport = {};
    m_scissor = {};
    m_state.reset();
    m_binding_set.reset();
    m_argument_tables = {};
    [m_residency_set commit];
    m_residency_set = nullptr;
    m_need_apply_state = false;
    m_need_apply_binding_set = false;
    m_render_barrier_after_stages = 0;
    m_render_barrier_before_stages = 0;
    m_compute_barrier_after_stages = 0;
    m_compute_barrier_before_stages = 0;
    m_closed = true;
}

void MTCommandList::BindPipeline(const std::shared_ptr<Pipeline>& state)
{
    m_state = state;
    m_need_apply_state = true;
    m_need_apply_binding_set = true;
}

void MTCommandList::BindBindingSet(const std::shared_ptr<BindingSet>& binding_set)
{
    m_binding_set = std::static_pointer_cast<MTBindingSet>(binding_set);
    if (m_binding_set) {
        m_binding_set->AddResourcesToResidencySet(m_residency_set);
    }
    m_need_apply_binding_set = true;
}

void MTCommandList::BeginRenderPass(const std::shared_ptr<RenderPass>& render_pass,
                                    const std::shared_ptr<Framebuffer>& framebuffer,
                                    const ClearDesc& clear_desc)
{
    CloseComputeEncoder();

    MTL4RenderPassDescriptor* render_pass_descriptor = [MTL4RenderPassDescriptor new];
    const RenderPassDesc& render_pass_desc = render_pass->GetDesc();
    const FramebufferDesc& framebuffer_desc = framebuffer->As<FramebufferBase>().GetDesc();

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

        render_pass_descriptor.renderTargetArrayLength =
            std::max<uint32_t>(render_pass_descriptor.renderTargetArrayLength, view->GetLayerCount());

        if (attachment.texture) {
            AddAllocation(attachment.texture);
        }
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

    render_pass_descriptor.renderTargetWidth = framebuffer_desc.width;
    render_pass_descriptor.renderTargetHeight = framebuffer_desc.height;
    render_pass_descriptor.defaultRasterSampleCount = render_pass_desc.sample_count;

    m_render_encoder = [m_command_buffer renderCommandEncoderWithDescriptor:render_pass_descriptor];
    if (m_render_encoder == nullptr) {
        NSLog(@"Error: failed to create render pass");
    }

    [m_render_encoder setViewport:m_viewport];
    [m_render_encoder setScissorRect:m_scissor];
    [m_render_encoder setArgumentTable:m_argument_tables.at(ShaderType::kVertex) atStages:MTLRenderStageVertex];
    [m_render_encoder setArgumentTable:m_argument_tables.at(ShaderType::kPixel) atStages:MTLRenderStageFragment];
    [m_render_encoder setArgumentTable:m_argument_tables.at(ShaderType::kAmplification) atStages:MTLRenderStageObject];
    [m_render_encoder setArgumentTable:m_argument_tables.at(ShaderType::kMesh) atStages:MTLRenderStageMesh];

    m_need_apply_state = true;
}

void MTCommandList::EndRenderPass()
{
    [m_render_encoder endEncoding];
    m_render_encoder = nullptr;
}

void MTCommandList::BeginEvent(const std::string& name) {}

void MTCommandList::EndEvent() {}

void MTCommandList::Draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
{
    ApplyGraphicsState();
    [m_render_encoder drawPrimitives:MTLPrimitiveTypeTriangle
                         vertexStart:first_vertex
                         vertexCount:vertex_count
                       instanceCount:instance_count
                        baseInstance:first_instance];
}

void MTCommandList::DrawIndexed(uint32_t index_count,
                                uint32_t instance_count,
                                uint32_t first_index,
                                int32_t vertex_offset,
                                uint32_t first_instance)
{
    ApplyGraphicsState();
    MTLIndexType index_format = ConvertIndexType(m_index_format);
    const uint32_t index_stride = index_format == MTLIndexTypeUInt32 ? 4 : 2;
    [m_render_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                 indexCount:index_count
                                  indexType:index_format
                                indexBuffer:m_index_buffer.gpuAddress + index_stride * first_index
                          indexBufferLength:m_index_buffer.length - index_stride * first_index
                              instanceCount:instance_count
                                 baseVertex:vertex_offset
                               baseInstance:first_instance];
}

void MTCommandList::DrawIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset)
{
    ApplyGraphicsState();
    decltype(auto) mt_argument_buffer = argument_buffer->As<MTResource>().buffer.res;
    AddAllocation(mt_argument_buffer);
    [m_render_encoder drawPrimitives:MTLPrimitiveTypeTriangle
                      indirectBuffer:mt_argument_buffer.gpuAddress + argument_buffer_offset];
}

void MTCommandList::DrawIndexedIndirect(const std::shared_ptr<Resource>& argument_buffer,
                                        uint64_t argument_buffer_offset)
{
    ApplyGraphicsState();
    decltype(auto) mt_argument_buffer = argument_buffer->As<MTResource>().buffer.res;
    AddAllocation(mt_argument_buffer);
    MTLIndexType index_format = ConvertIndexType(m_index_format);
    [m_render_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                  indexType:index_format
                                indexBuffer:m_index_buffer.gpuAddress
                          indexBufferLength:m_index_buffer.length
                             indirectBuffer:mt_argument_buffer.gpuAddress + argument_buffer_offset];
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
    decltype(auto) mt_state = m_state->As<MTComputePipeline>();
    MTLSize threadgroups_per_grid = { thread_group_count_x, thread_group_count_y, thread_group_count_z };

    OpenComputeEncoder();
    ApplyComputeState();
    AddComputeBarriers();
    [m_compute_encoder dispatchThreadgroups:threadgroups_per_grid threadsPerThreadgroup:mt_state.GetNumthreads()];
}

void MTCommandList::DispatchIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset)
{
    decltype(auto) mt_argument_buffer = argument_buffer->As<MTResource>().buffer.res;
    decltype(auto) mt_state = m_state->As<MTComputePipeline>();
    AddAllocation(mt_argument_buffer);

    OpenComputeEncoder();
    ApplyComputeState();
    AddComputeBarriers();
    [m_compute_encoder dispatchThreadgroupsWithIndirectBuffer:mt_argument_buffer.gpuAddress + argument_buffer_offset
                                        threadsPerThreadgroup:mt_state.GetNumthreads()];
}

void MTCommandList::DispatchMesh(uint32_t thread_group_count_x,
                                 uint32_t thread_group_count_y,
                                 uint32_t thread_group_count_z)
{
    ApplyGraphicsState();
    decltype(auto) mt_state = m_state->As<MTGraphicsPipeline>();
    [m_render_encoder drawMeshThreadgroups:MTLSizeMake(thread_group_count_x, thread_group_count_y, thread_group_count_z)
               threadsPerObjectThreadgroup:mt_state.GetAmplificationNumthreads()
                 threadsPerMeshThreadgroup:mt_state.GetMeshNumthreads()];
}

void MTCommandList::DispatchRays(const RayTracingShaderTables& shader_tables,
                                 uint32_t width,
                                 uint32_t height,
                                 uint32_t depth)
{
    assert(false);
}

void MTCommandList::ResourceBarrier(const std::vector<ResourceBarrierDesc>& barriers)
{
    for (const auto& barrier : barriers) {
        m_render_barrier_after_stages |= ResourceStateToMTLStages(barrier.state_after);
        m_render_barrier_before_stages |= ResourceStateToMTLStages(barrier.state_before);
        m_compute_barrier_after_stages |= ResourceStateToMTLStages(barrier.state_after);
        m_compute_barrier_before_stages |= ResourceStateToMTLStages(barrier.state_before);
    }
}

void MTCommandList::UAVResourceBarrier(const std::shared_ptr<Resource>& /*resource*/)
{
    m_render_barrier_after_stages |= MTLStageVertex | MTLStageObject | MTLStageMesh | MTLStageFragment;
    m_render_barrier_before_stages |= MTLStageVertex | MTLStageObject | MTLStageMesh | MTLStageFragment;
    m_compute_barrier_after_stages |= MTLStageDispatch;
    m_compute_barrier_before_stages |= MTLStageDispatch;
}

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

    [m_render_encoder setViewport:m_viewport];
}

void MTCommandList::SetScissorRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
{
    m_scissor.x = left;
    m_scissor.y = top;
    m_scissor.width = right - left;
    m_scissor.height = bottom - top;

    if (!m_render_encoder) {
        return;
    }

    [m_render_encoder setScissorRect:m_scissor];
}

void MTCommandList::IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format)
{
    decltype(auto) index_buffer = resource->As<MTResource>().buffer.res;
    AddAllocation(index_buffer);
    m_index_buffer = index_buffer;
    m_index_format = format;
}

void MTCommandList::IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource)
{
    decltype(auto) vertex = resource->As<MTResource>().buffer.res;
    AddAllocation(vertex);
    uint32_t index = m_device.GetMaxPerStageBufferCount() - slot - 1;
    [m_argument_tables.at(ShaderType::kVertex) setAddress:vertex.gpuAddress atIndex:index];
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

    MTL4PrimitiveAccelerationStructureDescriptor* acceleration_structure_desc =
        [MTL4PrimitiveAccelerationStructureDescriptor new];
    acceleration_structure_desc.geometryDescriptors = geometry_descs;

    decltype(auto) mt_dst = dst->As<MTResource>();
    decltype(auto) mt_scratch = scratch->As<MTResource>();
    AddAllocation(mt_dst.acceleration_structure);
    AddAllocation(mt_scratch.buffer.res);

    OpenComputeEncoder();
    AddComputeBarriers();
    [m_compute_encoder buildAccelerationStructure:mt_dst.acceleration_structure
                                       descriptor:acceleration_structure_desc
                                    scratchBuffer:MTL4BufferRangeMake(mt_scratch.buffer.res.gpuAddress + scratch_offset,
                                                                      mt_scratch.buffer.res.length - scratch_offset)];
}

// TODO: patch on GPU
MTL4BufferRange MTCommandList::PatchInstanceData(const std::shared_ptr<Resource>& instance_data,
                                                 uint64_t instance_offset,
                                                 uint32_t instance_count)
{
    MTLResourceOptions buffer_options = MTLStorageModeShared << MTLResourceStorageModeShift;
    NSUInteger buffer_size = instance_count * sizeof(MTLIndirectAccelerationStructureInstanceDescriptor);
    id<MTLBuffer> buffer = [m_device.GetDevice() newBufferWithLength:buffer_size options:buffer_options];
    AddAllocation(buffer);

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

    return MTL4BufferRangeMake(buffer.gpuAddress, buffer.length);
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
    MTL4InstanceAccelerationStructureDescriptor* acceleration_structure_desc =
        [MTL4InstanceAccelerationStructureDescriptor new];
    acceleration_structure_desc.instanceCount = instance_count;
    acceleration_structure_desc.instanceDescriptorBuffer =
        PatchInstanceData(instance_data, instance_offset, instance_count);
    acceleration_structure_desc.instanceDescriptorType = MTLAccelerationStructureInstanceDescriptorTypeIndirect;

    decltype(auto) mt_dst = dst->As<MTResource>();
    decltype(auto) mt_scratch = scratch->As<MTResource>();
    AddAllocation(mt_dst.acceleration_structure);
    AddAllocation(mt_scratch.buffer.res);

    OpenComputeEncoder();
    AddComputeBarriers();
    [m_compute_encoder buildAccelerationStructure:mt_dst.acceleration_structure
                                       descriptor:acceleration_structure_desc
                                    scratchBuffer:MTL4BufferRangeMake(mt_scratch.buffer.res.gpuAddress + scratch_offset,
                                                                      mt_scratch.buffer.res.length - scratch_offset)];
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
    OpenComputeEncoder();
    decltype(auto) mt_src_buffer = src_buffer->As<MTResource>();
    decltype(auto) mt_dst_buffer = dst_buffer->As<MTResource>();
    AddAllocation(mt_src_buffer.buffer.res);
    AddAllocation(mt_dst_buffer.buffer.res);
    AddComputeBarriers();
    for (const auto& region : regions) {
        [m_compute_encoder copyFromBuffer:mt_src_buffer.buffer.res
                             sourceOffset:region.src_offset
                                 toBuffer:mt_dst_buffer.buffer.res
                        destinationOffset:region.dst_offset
                                     size:region.num_bytes];
    }
}

void MTCommandList::CopyBufferToTexture(const std::shared_ptr<Resource>& src_buffer,
                                        const std::shared_ptr<Resource>& dst_texture,
                                        const std::vector<BufferToTextureCopyRegion>& regions)
{
    OpenComputeEncoder();
    decltype(auto) mt_src_buffer = src_buffer->As<MTResource>();
    decltype(auto) mt_dst_texture = dst_texture->As<MTResource>();
    AddAllocation(mt_src_buffer.buffer.res);
    AddAllocation(mt_dst_texture.texture.res);
    auto format = dst_texture->GetFormat();
    AddComputeBarriers();
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
        [m_compute_encoder copyFromBuffer:mt_src_buffer.buffer.res
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
}

void MTCommandList::CopyTexture(const std::shared_ptr<Resource>& src_texture,
                                const std::shared_ptr<Resource>& dst_texture,
                                const std::vector<TextureCopyRegion>& regions)
{
    OpenComputeEncoder();
    decltype(auto) mt_src_texture = src_texture->As<MTResource>();
    decltype(auto) mt_dst_texture = dst_texture->As<MTResource>();
    AddAllocation(mt_src_texture.texture.res);
    AddAllocation(mt_dst_texture.texture.res);
    auto format = dst_texture->GetFormat();
    AddComputeBarriers();
    for (const auto& region : regions) {
        MTLSize region_size = { region.extent.width, region.extent.height, region.extent.depth };
        MTLOrigin src_origin = { (uint32_t)region.src_offset.x, (uint32_t)region.src_offset.y,
                                 (uint32_t)region.src_offset.z };
        MTLOrigin dst_origin = { (uint32_t)region.dst_offset.x, (uint32_t)region.dst_offset.y,
                                 (uint32_t)region.dst_offset.z };
        [m_compute_encoder copyFromTexture:mt_src_texture.texture.res
                               sourceSlice:region.src_array_layer
                               sourceLevel:region.src_mip_level
                              sourceOrigin:src_origin
                                sourceSize:region_size
                                 toTexture:mt_dst_texture.texture.res
                          destinationSlice:region.dst_array_layer
                          destinationLevel:region.dst_mip_level
                         destinationOrigin:dst_origin];
    }
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

void MTCommandList::SetName(const std::string& name)
{
    m_command_buffer.label = [NSString stringWithUTF8String:name.c_str()];
}

id<MTL4CommandBuffer> MTCommandList::GetCommandBuffer()
{
    return m_command_buffer;
}

void MTCommandList::ApplyComputeState()
{
    if (m_need_apply_binding_set) {
        m_binding_set->Apply(m_argument_tables, m_state);
        m_need_apply_binding_set = false;
    }

    if (m_need_apply_state) {
        assert(m_state->GetPipelineType() == PipelineType::kCompute);
        decltype(auto) mt_state = m_state->As<MTComputePipeline>();
        [m_compute_encoder setComputePipelineState:mt_state.GetPipeline()];
    }
}

void MTCommandList::ApplyGraphicsState()
{
    if (m_need_apply_binding_set) {
        m_binding_set->Apply(m_argument_tables, m_state);
        m_need_apply_binding_set = false;
    }

    if (m_need_apply_state) {
        assert(m_state->GetPipelineType() == PipelineType::kGraphics);
        decltype(auto) mt_state = m_state->As<MTGraphicsPipeline>();
        decltype(auto) rasterizer_desc = mt_state.GetDesc().rasterizer_desc;
        [m_render_encoder setRenderPipelineState:mt_state.GetPipeline()];
        [m_render_encoder setDepthStencilState:mt_state.GetDepthStencil()];
        [m_render_encoder setDepthBias:rasterizer_desc.depth_bias slopeScale:0 clamp:0];
        [m_render_encoder setCullMode:ConvertCullMode(rasterizer_desc.cull_mode)];
        m_need_apply_state = false;
    }

    AddGraphicsBarriers();
}

void MTCommandList::AddGraphicsBarriers()
{
    if (m_render_barrier_after_stages && m_render_barrier_before_stages) {
        [m_render_encoder barrierAfterQueueStages:m_render_barrier_after_stages
                                     beforeStages:m_render_barrier_before_stages
                                visibilityOptions:MTL4VisibilityOptionNone];
        m_render_barrier_after_stages = 0;
        m_render_barrier_before_stages = 0;
    }
}

void MTCommandList::AddComputeBarriers()
{
    if (m_compute_barrier_after_stages && m_compute_barrier_before_stages) {
        [m_compute_encoder barrierAfterQueueStages:m_compute_barrier_after_stages
                                      beforeStages:m_compute_barrier_before_stages
                                 visibilityOptions:MTL4VisibilityOptionNone];
        m_compute_barrier_after_stages = 0;
        m_compute_barrier_before_stages = 0;
    }
}

void MTCommandList::CreateArgumentTables()
{
    m_argument_tables[ShaderType::kVertex] = CreateArgumentTable(m_device);
    m_argument_tables[ShaderType::kPixel] = CreateArgumentTable(m_device);
    m_argument_tables[ShaderType::kAmplification] = CreateArgumentTable(m_device);
    m_argument_tables[ShaderType::kMesh] = CreateArgumentTable(m_device);
    m_argument_tables[ShaderType::kCompute] = CreateArgumentTable(m_device);
}

void MTCommandList::AddAllocation(id<MTLAllocation> allocation)
{
    [m_residency_set addAllocation:allocation];
}

void MTCommandList::OpenComputeEncoder()
{
    if (m_compute_encoder) {
        return;
    }

    assert(!m_render_encoder);
    m_compute_encoder = [m_command_buffer computeCommandEncoder];
    [m_compute_encoder setArgumentTable:m_argument_tables.at(ShaderType::kCompute)];
    m_need_apply_state = true;
}

void MTCommandList::CloseComputeEncoder()
{
    if (!m_compute_encoder) {
        return;
    }

    [m_compute_encoder endEncoding];
    m_compute_encoder = nullptr;
}
