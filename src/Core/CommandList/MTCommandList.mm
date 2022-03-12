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

    decltype(auto) mtl_pipeline = m_state->GetPipeline();
    ApplyAndRecord([=] {
        [m_render_encoder setRenderPipelineState:mtl_pipeline];
    });
}

void MTCommandList::BindBindingSet(const std::shared_ptr<BindingSet>& binding_set)
{
    if (binding_set == m_binding_set)
        return;
    m_binding_set = std::static_pointer_cast<MTBindingSet>(binding_set);
    if (!m_render_encoder)
        return;
    
    ApplyAndRecord([=] {
        m_binding_set->Apply(m_render_encoder, m_state);
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
    for (size_t i = 0; i < render_pass_desc.colors.size(); ++i)
    {
        decltype(auto) attachment = render_pass_descriptor.colorAttachments[i];
        decltype(auto) render_pass_color = render_pass_desc.colors[i];
        attachment.loadAction = Convert(render_pass_color.load_op);
        attachment.storeAction = Convert(render_pass_color.store_op);
    }
    for (size_t i = 0; i < clear_desc.colors.size(); ++i)
    {
        decltype(auto) attachment = render_pass_descriptor.colorAttachments[i];
        attachment.clearColor = MTLClearColorMake(clear_desc.colors[i].r,
                                                  clear_desc.colors[i].g,
                                                  clear_desc.colors[i].b,
                                                  clear_desc.colors[i].a);
    }

    const FramebufferDesc& framebuffer_desc = framebuffer->As<FramebufferBase>().GetDesc();
    for (size_t i = 0; i < render_pass_desc.colors.size(); ++i)
    {
        if (render_pass_desc.colors[i].format == gli::format::FORMAT_UNDEFINED)
            continue;
        decltype(auto) attachment = render_pass_descriptor.colorAttachments[i];
        decltype(auto) mt_view = framebuffer_desc.colors[i]->As<MTView>();
        attachment.level = mt_view.GetBaseMipLevel();
        attachment.slice = mt_view.GetBaseArrayLayer();
        decltype(auto) resource = mt_view.GetResource();
        if (!resource)
            continue;
        decltype(auto) mt_resource = resource->As<MTResource>();
        attachment.texture = mt_resource.texture.res;
    }

    ApplyAndRecord([=] {
        m_render_encoder = [m_command_buffer renderCommandEncoderWithDescriptor:render_pass_descriptor];
        [m_render_encoder setViewport:m_viewport];
        if (m_state)
        {
            [m_render_encoder setRenderPipelineState:m_state->GetPipeline()];
        }
        for (const auto& vertex : m_vertices)
        {
            [m_render_encoder
                setVertexBuffer:vertex.second
                    offset:0 atIndex:vertex.first];
        }
        if (m_binding_set)
        {
            m_binding_set->Apply(m_render_encoder, m_state);
        }
    });
}

void MTCommandList::EndRenderPass()
{
    ApplyAndRecord([=] {
        [m_render_encoder endEncoding];
        m_render_encoder = nullptr;
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
    ApplyAndRecord([=] {
        [m_render_encoder
            drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                indexCount:index_count
                indexType:index_format
                indexBuffer:index
                indexBufferOffset:0
                instanceCount:1];
    });
}

void MTCommandList::DrawIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset)
{
}

void MTCommandList::DrawIndexedIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset)
{
}

void MTCommandList::DrawIndirectCount(
    const std::shared_ptr<Resource>& argument_buffer,
    uint64_t argument_buffer_offset,
    const std::shared_ptr<Resource>& count_buffer,
    uint64_t count_buffer_offset,
    uint32_t max_draw_count,
    uint32_t stride)
{
}

void MTCommandList::DrawIndexedIndirectCount(
    const std::shared_ptr<Resource>& argument_buffer,
    uint64_t argument_buffer_offset,
    const std::shared_ptr<Resource>& count_buffer,
    uint64_t count_buffer_offset,
    uint32_t max_draw_count,
    uint32_t stride)
{
}

void MTCommandList::Dispatch(uint32_t thread_group_count_x, uint32_t thread_group_count_y, uint32_t thread_group_count_z)
{
}

void MTCommandList::DispatchIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset)
{
}

void MTCommandList::DispatchMesh(uint32_t thread_group_count_x)
{
}

void MTCommandList::DispatchRays(const RayTracingShaderTables& shader_tables, uint32_t width, uint32_t height, uint32_t depth)
{
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

    ApplyAndRecord([=] {
        [m_render_encoder setViewport:m_viewport];
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

    ApplyAndRecord([=] {
        [m_render_encoder
            setVertexBuffer:vertex
                offset:0 atIndex:index];
    });
}

void MTCommandList::RSSetShadingRate(ShadingRate shading_rate, const std::array<ShadingRateCombiner, 2>& combiners)
{
}

void MTCommandList::BuildBottomLevelAS(
    const std::shared_ptr<Resource>& src,
    const std::shared_ptr<Resource>& dst,
    const std::shared_ptr<Resource>& scratch,
    uint64_t scratch_offset,
    const std::vector<RaytracingGeometryDesc>& descs,
    BuildAccelerationStructureFlags flags)
{
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
}

void MTCommandList::CopyAccelerationStructure(const std::shared_ptr<Resource>& src, const std::shared_ptr<Resource>& dst, CopyAccelerationStructureMode mode)
{
}

void MTCommandList::CopyBuffer(const std::shared_ptr<Resource>& src_buffer, const std::shared_ptr<Resource>& dst_buffer,
                               const std::vector<BufferCopyRegion>& regions)
{
}

void MTCommandList::CopyBufferToTexture(const std::shared_ptr<Resource>& src_buffer, const std::shared_ptr<Resource>& dst_texture,
                                        const std::vector<BufferToTextureCopyRegion>& regions)
{
}

void MTCommandList::CopyTexture(const std::shared_ptr<Resource>& src_texture, const std::shared_ptr<Resource>& dst_texture,
                                const std::vector<TextureCopyRegion>& regions)
{
}

void MTCommandList::WriteAccelerationStructuresProperties(
    const std::vector<std::shared_ptr<Resource>>& acceleration_structures,
    const std::shared_ptr<QueryHeap>& query_heap,
    uint32_t first_query)
{
}

void MTCommandList::ResolveQueryData(
    const std::shared_ptr<QueryHeap>& query_heap,
    uint32_t first_query,
    uint32_t query_count,
    const std::shared_ptr<Resource>& dst_buffer,
    uint64_t dst_offset)
{
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
