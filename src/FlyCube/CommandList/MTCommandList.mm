#include "CommandList/MTCommandList.h"

#include "BindingSet/MTBindingSet.h"
#include "Device/MTDevice.h"
#include "Pipeline/MTComputePipeline.h"
#include "Pipeline/MTGraphicsPipeline.h"
#include "QueryHeap/MTQueryHeap.h"
#include "Resource/MTResource.h"
#include "Utilities/Logging.h"
#include "Utilities/NotReached.h"
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
        NOTREACHED();
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
        NOTREACHED();
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
        NOTREACHED();
    }
}

MTLTriangleFillMode ConvertFillMode(FillMode fill_mode)
{
    switch (fill_mode) {
    case FillMode::kSolid:
        return MTLTriangleFillModeFill;
    case FillMode::kWireframe:
        return MTLTriangleFillModeLines;
    default:
        NOTREACHED();
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
        NOTREACHED();
    }
}

MTLWinding ConvertFrontFace(FrontFace front_face)
{
    switch (front_face) {
    case FrontFace::kClockwise:
        return MTLWindingClockwise;
    case FrontFace::kCounterClockwise:
        return MTLWindingCounterClockwise;
    default:
        NOTREACHED();
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
    id<MTL4ArgumentTable> argument_table = [device.GetDevice() newArgumentTableWithDescriptor:argument_table_descriptor
                                                                                        error:&error];
    if (!argument_table) {
        Logging::Println("Failed to create MTL4ArgumentTable: {}", error);
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
    : device_(device)
{
    Reset();
}

void MTCommandList::Reset()
{
    Close();
    closed_ = false;

    command_buffer_ = [device_.GetDevice() newCommandBuffer];
    if (!allocator_) {
        allocator_ = [device_.GetDevice() newCommandAllocator];
    } else {
        [allocator_ reset];
    }
    [command_buffer_ beginCommandBufferWithAllocator:allocator_];

    CreateArgumentTables();
    residency_set_ = device_.CreateResidencySet();
    [command_buffer_ useResidencySet:residency_set_];

    static constexpr MTLStages kRenderStages = MTLStageVertex | MTLStageObject | MTLStageMesh | MTLStageFragment;
    static constexpr MTLStages kComputeStages = MTLStageDispatch | MTLStageBlit | MTLStageAccelerationStructure;

    render_barrier_after_stages_ = MTLStageAll;
    render_barrier_before_stages_ = kRenderStages;
    compute_barrier_after_stages_ = MTLStageAll;
    compute_barrier_before_stages_ = kComputeStages;
}

void MTCommandList::Close()
{
    if (closed_) {
        return;
    }

    CloseComputeEncoder();

    [command_buffer_ endCommandBuffer];
    render_encoder_ = nullptr;
    index_buffer_ = nullptr;
    index_buffer_offset_ = 0;
    index_format_ = gli::FORMAT_UNDEFINED;
    viewport_ = {};
    scissor_ = {};
    min_depth_bounds_ = 0.0;
    max_depth_bounds_ = 1.0;
    stencil_reference_ = 0;
    state_.reset();
    binding_set_.reset();
    argument_tables_ = {};
    [residency_set_ commit];
    residency_set_ = nullptr;
    patch_buffers_.clear();
    need_apply_state_ = false;
    need_apply_binding_set_ = false;
    render_barrier_after_stages_ = 0;
    render_barrier_before_stages_ = 0;
    compute_barrier_after_stages_ = 0;
    compute_barrier_before_stages_ = 0;
    closed_ = true;
}

void MTCommandList::BindPipeline(const std::shared_ptr<Pipeline>& state)
{
    state_ = state;
    need_apply_state_ = true;
    need_apply_binding_set_ = true;
}

void MTCommandList::BindBindingSet(const std::shared_ptr<BindingSet>& binding_set)
{
    binding_set_ = std::static_pointer_cast<MTBindingSet>(binding_set);
    need_apply_binding_set_ = true;
}

void MTCommandList::BeginRenderPass(const RenderPassDesc& render_pass_desc)
{
    CloseComputeEncoder();

    auto add_attachment = [&](auto& attachment, RenderPassLoadOp load_op, RenderPassStoreOp store_op,
                              const std::shared_ptr<View>& view) {
        if (!view) {
            return;
        }
        attachment.loadAction = ConvertLoadAction(load_op);
        attachment.storeAction = ConvertStoreAction(store_op);

        decltype(auto) mt_view = view->As<MTView>();
        attachment.level = mt_view.GetBaseMipLevel();
        attachment.slice = mt_view.GetBaseArrayLayer();
        attachment.texture = mt_view.GetTexture();

        if (attachment.texture) {
            AddAllocation(attachment.texture);
        }
    };

    MTL4RenderPassDescriptor* render_pass_descriptor = [MTL4RenderPassDescriptor new];

    for (size_t i = 0; i < render_pass_desc.colors.size(); ++i) {
        decltype(auto) color_attachment = render_pass_descriptor.colorAttachments[i];
        add_attachment(color_attachment, render_pass_desc.colors[i].load_op, render_pass_desc.colors[i].store_op,
                       render_pass_desc.colors[i].view);
        if (render_pass_desc.colors[i].load_op == RenderPassLoadOp::kClear) {
            color_attachment.clearColor =
                MTLClearColorMake(render_pass_desc.colors[i].clear_value.r, render_pass_desc.colors[i].clear_value.g,
                                  render_pass_desc.colors[i].clear_value.b, render_pass_desc.colors[i].clear_value.a);
        }
    }

    decltype(auto) depth_attachment = render_pass_descriptor.depthAttachment;
    if (render_pass_desc.depth_stencil_view &&
        gli::is_depth(render_pass_desc.depth_stencil_view->GetResource()->GetFormat())) {
        add_attachment(depth_attachment, render_pass_desc.depth.load_op, render_pass_desc.depth.store_op,
                       render_pass_desc.depth_stencil_view);
    }
    depth_attachment.clearDepth = render_pass_desc.depth.clear_value;

    decltype(auto) stencil_attachment = render_pass_descriptor.stencilAttachment;
    if (render_pass_desc.depth_stencil_view &&
        gli::is_stencil(render_pass_desc.depth_stencil_view->GetResource()->GetFormat())) {
        add_attachment(stencil_attachment, render_pass_desc.stencil.load_op, render_pass_desc.stencil.store_op,
                       render_pass_desc.depth_stencil_view);
    }
    stencil_attachment.clearStencil = render_pass_desc.stencil.clear_value;

    render_pass_descriptor.renderTargetWidth = render_pass_desc.render_area.x + render_pass_desc.render_area.width;
    render_pass_descriptor.renderTargetHeight = render_pass_desc.render_area.y + render_pass_desc.render_area.height;
    render_pass_descriptor.renderTargetArrayLength = render_pass_desc.layers;
    render_pass_descriptor.defaultRasterSampleCount = render_pass_desc.sample_count;

    render_encoder_ = [command_buffer_ renderCommandEncoderWithDescriptor:render_pass_descriptor];
    if (render_encoder_ == nullptr) {
        Logging::Println("Failed to create MTL4RenderCommandEncoder");
    }

    [render_encoder_ setViewport:viewport_];
    [render_encoder_ setScissorRect:scissor_];
    if (min_depth_bounds_ != 0.0 || max_depth_bounds_ != 1.0) {
        [render_encoder_ setDepthTestMinBound:min_depth_bounds_ maxBound:max_depth_bounds_];
    }
    if (stencil_reference_) {
        [render_encoder_ setStencilReferenceValue:stencil_reference_];
    }
    [render_encoder_ setArgumentTable:argument_tables_.at(ShaderType::kVertex) atStages:MTLRenderStageVertex];
    [render_encoder_ setArgumentTable:argument_tables_.at(ShaderType::kPixel) atStages:MTLRenderStageFragment];
    [render_encoder_ setArgumentTable:argument_tables_.at(ShaderType::kAmplification) atStages:MTLRenderStageObject];
    [render_encoder_ setArgumentTable:argument_tables_.at(ShaderType::kMesh) atStages:MTLRenderStageMesh];

    need_apply_state_ = true;
}

void MTCommandList::EndRenderPass()
{
    [render_encoder_ endEncoding];
    render_encoder_ = nullptr;
}

void MTCommandList::BeginEvent(const std::string& name) {}

void MTCommandList::EndEvent() {}

void MTCommandList::Draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
{
    ApplyGraphicsState();
    [render_encoder_ drawPrimitives:MTLPrimitiveTypeTriangle
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
    MTLIndexType index_format = ConvertIndexType(index_format_);
    const uint32_t index_stride = index_format == MTLIndexTypeUInt32 ? 4 : 2;
    [render_encoder_ drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                indexCount:index_count
                                 indexType:index_format
                               indexBuffer:index_buffer_.gpuAddress + index_buffer_offset_ + index_stride * first_index
                         indexBufferLength:index_buffer_.length - index_buffer_offset_ - index_stride * first_index
                             instanceCount:instance_count
                                baseVertex:vertex_offset
                              baseInstance:first_instance];
}

void MTCommandList::DrawIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset)
{
    ApplyGraphicsState();
    decltype(auto) mt_argument_buffer = argument_buffer->As<MTResource>().GetBuffer();
    AddAllocation(mt_argument_buffer);
    [render_encoder_ drawPrimitives:MTLPrimitiveTypeTriangle
                     indirectBuffer:mt_argument_buffer.gpuAddress + argument_buffer_offset];
}

void MTCommandList::DrawIndexedIndirect(const std::shared_ptr<Resource>& argument_buffer,
                                        uint64_t argument_buffer_offset)
{
    ApplyGraphicsState();
    decltype(auto) mt_argument_buffer = argument_buffer->As<MTResource>().GetBuffer();
    AddAllocation(mt_argument_buffer);
    MTLIndexType index_format = ConvertIndexType(index_format_);
    [render_encoder_ drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                 indexType:index_format
                               indexBuffer:index_buffer_.gpuAddress + index_buffer_offset_
                         indexBufferLength:index_buffer_.length - index_buffer_offset_
                            indirectBuffer:mt_argument_buffer.gpuAddress + argument_buffer_offset];
}

void MTCommandList::DrawIndirectCount(const std::shared_ptr<Resource>& argument_buffer,
                                      uint64_t argument_buffer_offset,
                                      const std::shared_ptr<Resource>& count_buffer,
                                      uint64_t count_buffer_offset,
                                      uint32_t max_draw_count,
                                      uint32_t stride)
{
    NOTREACHED();
}

void MTCommandList::DrawIndexedIndirectCount(const std::shared_ptr<Resource>& argument_buffer,
                                             uint64_t argument_buffer_offset,
                                             const std::shared_ptr<Resource>& count_buffer,
                                             uint64_t count_buffer_offset,
                                             uint32_t max_draw_count,
                                             uint32_t stride)
{
    NOTREACHED();
}

void MTCommandList::Dispatch(uint32_t thread_group_count_x,
                             uint32_t thread_group_count_y,
                             uint32_t thread_group_count_z)
{
    decltype(auto) mt_state = state_->As<MTComputePipeline>();
    MTLSize threadgroups_per_grid = { thread_group_count_x, thread_group_count_y, thread_group_count_z };

    OpenComputeEncoder();
    ApplyComputeState();
    AddComputeBarriers();
    [compute_encoder_ dispatchThreadgroups:threadgroups_per_grid threadsPerThreadgroup:mt_state.GetNumthreads()];
}

void MTCommandList::DispatchIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset)
{
    decltype(auto) mt_argument_buffer = argument_buffer->As<MTResource>().GetBuffer();
    decltype(auto) mt_state = state_->As<MTComputePipeline>();
    AddAllocation(mt_argument_buffer);

    OpenComputeEncoder();
    ApplyComputeState();
    AddComputeBarriers();
    [compute_encoder_ dispatchThreadgroupsWithIndirectBuffer:mt_argument_buffer.gpuAddress + argument_buffer_offset
                                       threadsPerThreadgroup:mt_state.GetNumthreads()];
}

void MTCommandList::DispatchMesh(uint32_t thread_group_count_x,
                                 uint32_t thread_group_count_y,
                                 uint32_t thread_group_count_z)
{
    ApplyGraphicsState();
    decltype(auto) mt_state = state_->As<MTGraphicsPipeline>();
    [render_encoder_ drawMeshThreadgroups:MTLSizeMake(thread_group_count_x, thread_group_count_y, thread_group_count_z)
              threadsPerObjectThreadgroup:mt_state.GetAmplificationNumthreads()
                threadsPerMeshThreadgroup:mt_state.GetMeshNumthreads()];
}

void MTCommandList::DispatchRays(const RayTracingShaderTables& shader_tables,
                                 uint32_t width,
                                 uint32_t height,
                                 uint32_t depth)
{
    NOTREACHED();
}

void MTCommandList::ResourceBarrier(const std::vector<ResourceBarrierDesc>& barriers)
{
    for (const auto& barrier : barriers) {
        render_barrier_after_stages_ |= ResourceStateToMTLStages(barrier.state_after);
        render_barrier_before_stages_ |= ResourceStateToMTLStages(barrier.state_before);
        compute_barrier_after_stages_ |= ResourceStateToMTLStages(barrier.state_after);
        compute_barrier_before_stages_ |= ResourceStateToMTLStages(barrier.state_before);
    }
}

void MTCommandList::UAVResourceBarrier(const std::shared_ptr<Resource>& /*resource*/)
{
    render_barrier_after_stages_ |= MTLStageVertex | MTLStageObject | MTLStageMesh | MTLStageFragment;
    render_barrier_before_stages_ |= MTLStageVertex | MTLStageObject | MTLStageMesh | MTLStageFragment;
    compute_barrier_after_stages_ |= MTLStageDispatch;
    compute_barrier_before_stages_ |= MTLStageDispatch;
}

void MTCommandList::SetViewport(float x, float y, float width, float height, float min_depth, float max_depth)
{
    viewport_.originX = x;
    viewport_.originY = y;
    viewport_.width = width;
    viewport_.height = height;
    viewport_.znear = min_depth;
    viewport_.zfar = max_depth;

    if (!render_encoder_) {
        return;
    }

    [render_encoder_ setViewport:viewport_];
}

void MTCommandList::SetScissorRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
{
    scissor_.x = left;
    scissor_.y = top;
    scissor_.width = right - left;
    scissor_.height = bottom - top;

    if (!render_encoder_) {
        return;
    }

    [render_encoder_ setScissorRect:scissor_];
}

void MTCommandList::IASetIndexBuffer(const std::shared_ptr<Resource>& resource, uint64_t offset, gli::format format)
{
    decltype(auto) index_buffer = resource->As<MTResource>().GetBuffer();
    AddAllocation(index_buffer);
    index_buffer_ = index_buffer;
    index_buffer_offset_ = offset;
    index_format_ = format;
}

void MTCommandList::IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource, uint64_t offset)
{
    decltype(auto) vertex = resource->As<MTResource>().GetBuffer();
    AddAllocation(vertex);
    uint32_t index = device_.GetMaxPerStageBufferCount() - slot - 1;
    [argument_tables_.at(ShaderType::kVertex) setAddress:vertex.gpuAddress + offset atIndex:index];
}

void MTCommandList::RSSetShadingRate(ShadingRate shading_rate, const std::array<ShadingRateCombiner, 2>& combiners)
{
    NOTREACHED();
}

void MTCommandList::SetDepthBounds(float min_depth_bounds, float max_depth_bounds)
{
    min_depth_bounds_ = min_depth_bounds;
    max_depth_bounds_ = max_depth_bounds;

    if (!render_encoder_) {
        return;
    }

    if (min_depth_bounds_ != 0.0 || max_depth_bounds_ != 1.0) {
        [render_encoder_ setDepthTestMinBound:min_depth_bounds_ maxBound:max_depth_bounds_];
    }
}

void MTCommandList::SetStencilReference(uint32_t stencil_reference)
{
    stencil_reference_ = stencil_reference;

    if (!render_encoder_) {
        return;
    }

    [render_encoder_ setStencilReferenceValue:stencil_reference_];
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
        MTL4AccelerationStructureTriangleGeometryDescriptor* geometry_desc =
            FillRaytracingGeometryDesc(desc.vertex, desc.index, desc.flags);
        [geometry_descs addObject:geometry_desc];
    }

    MTL4PrimitiveAccelerationStructureDescriptor* acceleration_structure_desc =
        [MTL4PrimitiveAccelerationStructureDescriptor new];
    acceleration_structure_desc.geometryDescriptors = geometry_descs;

    decltype(auto) mt_dst = dst->As<MTResource>();
    decltype(auto) mt_scratch = scratch->As<MTResource>();
    AddAllocation(mt_dst.GetAccelerationStructure());
    AddAllocation(mt_scratch.GetBuffer());

    OpenComputeEncoder();
    AddComputeBarriers();
    [compute_encoder_ buildAccelerationStructure:mt_dst.GetAccelerationStructure()
                                      descriptor:acceleration_structure_desc
                                   scratchBuffer:MTL4BufferRangeMake(mt_scratch.GetBuffer().gpuAddress + scratch_offset,
                                                                     mt_scratch.GetBuffer().length - scratch_offset)];
}

// TODO: patch on GPU
id<MTLBuffer> MTCommandList::PatchInstanceData(const std::shared_ptr<Resource>& instance_data,
                                               uint64_t instance_offset,
                                               uint32_t instance_count)
{
    MTLResourceOptions buffer_options = MTLStorageModeShared << MTLResourceStorageModeShift;
    NSUInteger buffer_size = instance_count * sizeof(MTLIndirectAccelerationStructureInstanceDescriptor);
    id<MTLBuffer> buffer = [device_.GetDevice() newBufferWithLength:buffer_size options:buffer_options];
    AddAllocation(buffer);

    uint8_t* instance_ptr =
        static_cast<uint8_t*>(instance_data->As<MTResource>().GetBuffer().contents) + instance_offset;
    uint8_t* patched_instance_ptr = static_cast<uint8_t*>(buffer.contents);
    for (uint32_t i = 0; i < instance_count; ++i) {
        RaytracingGeometryInstance& instance = reinterpret_cast<RaytracingGeometryInstance*>(instance_ptr)[i];
        MTLIndirectAccelerationStructureInstanceDescriptor& patched_instance =
            reinterpret_cast<MTLIndirectAccelerationStructureInstanceDescriptor*>(patched_instance_ptr)[i];

        glm::mat4x3 matrix = glm::transpose(instance.transform);
        memcpy(&patched_instance.transformationMatrix, &matrix, sizeof(patched_instance.transformationMatrix));
        patched_instance.options = static_cast<MTLAccelerationStructureInstanceOptions>(instance.flags);
        patched_instance.mask = instance.instance_mask;
        patched_instance.userID = instance.instance_id;
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
    MTL4InstanceAccelerationStructureDescriptor* acceleration_structure_desc =
        [MTL4InstanceAccelerationStructureDescriptor new];
    acceleration_structure_desc.instanceCount = instance_count;
    id<MTLBuffer> patched_instance_data = PatchInstanceData(instance_data, instance_offset, instance_count);
    acceleration_structure_desc.instanceDescriptorBuffer =
        MTL4BufferRangeMake(patched_instance_data.gpuAddress, patched_instance_data.length);
    acceleration_structure_desc.instanceDescriptorType = MTLAccelerationStructureInstanceDescriptorTypeIndirect;
    patch_buffers_.push_back(patched_instance_data);

    decltype(auto) mt_dst = dst->As<MTResource>();
    decltype(auto) mt_scratch = scratch->As<MTResource>();
    AddAllocation(mt_dst.GetAccelerationStructure());
    AddAllocation(mt_scratch.GetBuffer());

    OpenComputeEncoder();
    AddComputeBarriers();
    [compute_encoder_ buildAccelerationStructure:mt_dst.GetAccelerationStructure()
                                      descriptor:acceleration_structure_desc
                                   scratchBuffer:MTL4BufferRangeMake(mt_scratch.GetBuffer().gpuAddress + scratch_offset,
                                                                     mt_scratch.GetBuffer().length - scratch_offset)];
}

void MTCommandList::CopyAccelerationStructure(const std::shared_ptr<Resource>& src,
                                              const std::shared_ptr<Resource>& dst,
                                              CopyAccelerationStructureMode mode)
{
    OpenComputeEncoder();
    AddComputeBarriers();
    id<MTLAccelerationStructure> mt_src_acceleration_structure = src->As<MTResource>().GetAccelerationStructure();
    id<MTLAccelerationStructure> mt_dst_acceleration_structure = dst->As<MTResource>().GetAccelerationStructure();
    AddAllocation(mt_src_acceleration_structure);
    AddAllocation(mt_dst_acceleration_structure);
    switch (mode) {
    case CopyAccelerationStructureMode::kClone:
        [compute_encoder_ copyAccelerationStructure:mt_src_acceleration_structure
                            toAccelerationStructure:mt_dst_acceleration_structure];
        break;
    case CopyAccelerationStructureMode::kCompact:
        [compute_encoder_ copyAndCompactAccelerationStructure:mt_src_acceleration_structure
                                      toAccelerationStructure:mt_dst_acceleration_structure];
        break;
    default:
        NOTREACHED();
    }
}

void MTCommandList::CopyBuffer(const std::shared_ptr<Resource>& src_buffer,
                               const std::shared_ptr<Resource>& dst_buffer,
                               const std::vector<BufferCopyRegion>& regions)
{
    OpenComputeEncoder();
    decltype(auto) mt_src_buffer = src_buffer->As<MTResource>();
    decltype(auto) mt_dst_buffer = dst_buffer->As<MTResource>();
    AddAllocation(mt_src_buffer.GetBuffer());
    AddAllocation(mt_dst_buffer.GetBuffer());
    AddComputeBarriers();
    for (const auto& region : regions) {
        [compute_encoder_ copyFromBuffer:mt_src_buffer.GetBuffer()
                            sourceOffset:region.src_offset
                                toBuffer:mt_dst_buffer.GetBuffer()
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
    AddAllocation(mt_src_buffer.GetBuffer());
    AddAllocation(mt_dst_texture.GetTexture());
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
        [compute_encoder_ copyFromBuffer:mt_src_buffer.GetBuffer()
                            sourceOffset:region.buffer_offset
                       sourceBytesPerRow:region.buffer_row_pitch
                     sourceBytesPerImage:bytes_per_image
                              sourceSize:region_size
                               toTexture:mt_dst_texture.GetTexture()
                        destinationSlice:region.texture_array_layer
                        destinationLevel:region.texture_mip_level
                       destinationOrigin:{ region.texture_offset.x, region.texture_offset.y, region.texture_offset.z }];
    }
}

void MTCommandList::CopyTexture(const std::shared_ptr<Resource>& src_texture,
                                const std::shared_ptr<Resource>& dst_texture,
                                const std::vector<TextureCopyRegion>& regions)
{
    OpenComputeEncoder();
    decltype(auto) mt_src_texture = src_texture->As<MTResource>();
    decltype(auto) mt_dst_texture = dst_texture->As<MTResource>();
    AddAllocation(mt_src_texture.GetTexture());
    AddAllocation(mt_dst_texture.GetTexture());
    auto format = dst_texture->GetFormat();
    AddComputeBarriers();
    for (const auto& region : regions) {
        MTLSize region_size = { region.extent.width, region.extent.height, region.extent.depth };
        MTLOrigin src_origin = { region.src_offset.x, region.src_offset.y, region.src_offset.z };
        MTLOrigin dst_origin = { region.dst_offset.x, region.dst_offset.y, region.dst_offset.z };
        [compute_encoder_ copyFromTexture:mt_src_texture.GetTexture()
                              sourceSlice:region.src_array_layer
                              sourceLevel:region.src_mip_level
                             sourceOrigin:src_origin
                               sourceSize:region_size
                                toTexture:mt_dst_texture.GetTexture()
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
    OpenComputeEncoder();
    AddComputeBarriers();
    id<MTLBuffer> mt_query_buffer = query_heap->As<MTQueryHeap>().GetBuffer();
    AddAllocation(mt_query_buffer);
    for (size_t i = 0; i < acceleration_structures.size(); ++i) {
        id<MTLAccelerationStructure> mt_acceleration_structure =
            acceleration_structures[i]->As<MTResource>().GetAccelerationStructure();
        AddAllocation(mt_acceleration_structure);
        [compute_encoder_
            writeCompactedAccelerationStructureSize:mt_acceleration_structure
                                           toBuffer:MTL4BufferRangeMake(mt_query_buffer.gpuAddress +
                                                                            (first_query + i) * sizeof(uint64_t),
                                                                        sizeof(uint64_t))];
    }
}

void MTCommandList::ResolveQueryData(const std::shared_ptr<QueryHeap>& query_heap,
                                     uint32_t first_query,
                                     uint32_t query_count,
                                     const std::shared_ptr<Resource>& dst_buffer,
                                     uint64_t dst_offset)
{
    if (compute_encoder_) {
        [compute_encoder_ barrierAfterEncoderStages:MTLStageAccelerationStructure
                                beforeEncoderStages:MTLStageBlit
                                  visibilityOptions:MTL4VisibilityOptionNone];
    } else {
        OpenComputeEncoder();
    }
    AddComputeBarriers();
    id<MTLBuffer> mt_query_buffer = query_heap->As<MTQueryHeap>().GetBuffer();
    id<MTLBuffer> mt_dst_buffer = dst_buffer->As<MTResource>().GetBuffer();
    AddAllocation(mt_query_buffer);
    AddAllocation(mt_dst_buffer);
    [compute_encoder_ copyFromBuffer:mt_query_buffer
                        sourceOffset:first_query * sizeof(uint64_t)
                            toBuffer:mt_dst_buffer
                   destinationOffset:dst_offset
                                size:sizeof(uint64_t) * query_count];
}

void MTCommandList::SetName(const std::string& name)
{
    command_buffer_.label = [NSString stringWithUTF8String:name.c_str()];
}

id<MTL4CommandBuffer> MTCommandList::GetCommandBuffer()
{
    return command_buffer_;
}

void MTCommandList::ApplyComputeState()
{
    if (need_apply_binding_set_ && binding_set_) {
        binding_set_->Apply(argument_tables_, state_, residency_set_);
        need_apply_binding_set_ = false;
    }

    if (need_apply_state_) {
        assert(state_->GetPipelineType() == PipelineType::kCompute);
        decltype(auto) mt_state = state_->As<MTComputePipeline>();
        [compute_encoder_ setComputePipelineState:mt_state.GetPipeline()];
    }
}

void MTCommandList::ApplyGraphicsState()
{
    if (need_apply_binding_set_ && binding_set_) {
        binding_set_->Apply(argument_tables_, state_, residency_set_);
        need_apply_binding_set_ = false;
    }

    if (need_apply_state_) {
        assert(state_->GetPipelineType() == PipelineType::kGraphics);
        decltype(auto) mt_state = state_->As<MTGraphicsPipeline>();
        decltype(auto) rasterizer_desc = mt_state.GetDesc().rasterizer_desc;
        [render_encoder_ setRenderPipelineState:mt_state.GetPipeline()];
        [render_encoder_ setTriangleFillMode:ConvertFillMode(rasterizer_desc.fill_mode)];
        [render_encoder_ setCullMode:ConvertCullMode(rasterizer_desc.cull_mode)];
        [render_encoder_ setFrontFacingWinding:ConvertFrontFace(rasterizer_desc.front_face)];
        [render_encoder_ setDepthBias:rasterizer_desc.depth_bias
                           slopeScale:rasterizer_desc.slope_scaled_depth_bias
                                clamp:rasterizer_desc.depth_bias_clamp];
        [render_encoder_
            setDepthClipMode:rasterizer_desc.depth_clip_enable ? MTLDepthClipModeClip : MTLDepthClipModeClamp];
        [render_encoder_ setDepthStencilState:mt_state.GetDepthStencil()];
        need_apply_state_ = false;
    }

    AddGraphicsBarriers();
}

void MTCommandList::AddGraphicsBarriers()
{
    if (render_barrier_after_stages_ && render_barrier_before_stages_) {
        [render_encoder_ barrierAfterQueueStages:render_barrier_after_stages_
                                    beforeStages:render_barrier_before_stages_
                               visibilityOptions:MTL4VisibilityOptionNone];
        render_barrier_after_stages_ = 0;
        render_barrier_before_stages_ = 0;
    }
}

void MTCommandList::AddComputeBarriers()
{
    if (compute_barrier_after_stages_ && compute_barrier_before_stages_) {
        [compute_encoder_ barrierAfterQueueStages:compute_barrier_after_stages_
                                     beforeStages:compute_barrier_before_stages_
                                visibilityOptions:MTL4VisibilityOptionNone];
        compute_barrier_after_stages_ = 0;
        compute_barrier_before_stages_ = 0;
    }
}

void MTCommandList::CreateArgumentTables()
{
    argument_tables_[ShaderType::kVertex] = CreateArgumentTable(device_);
    argument_tables_[ShaderType::kPixel] = CreateArgumentTable(device_);
    argument_tables_[ShaderType::kAmplification] = CreateArgumentTable(device_);
    argument_tables_[ShaderType::kMesh] = CreateArgumentTable(device_);
    argument_tables_[ShaderType::kCompute] = CreateArgumentTable(device_);
}

void MTCommandList::AddAllocation(id<MTLAllocation> allocation)
{
    [residency_set_ addAllocation:allocation];
}

void MTCommandList::OpenComputeEncoder()
{
    if (compute_encoder_) {
        return;
    }

    assert(!render_encoder_);
    compute_encoder_ = [command_buffer_ computeCommandEncoder];
    [compute_encoder_ setArgumentTable:argument_tables_.at(ShaderType::kCompute)];
    need_apply_state_ = true;
}

void MTCommandList::CloseComputeEncoder()
{
    if (!compute_encoder_) {
        return;
    }

    [compute_encoder_ endEncoding];
    compute_encoder_ = nullptr;
}
