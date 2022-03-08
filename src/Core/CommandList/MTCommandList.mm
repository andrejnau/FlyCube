#include "CommandList/MTCommandList.h"
#include <Device/MTDevice.h>

MTCommandList::MTCommandList(MTDevice& device, CommandListType type)
    : m_device(device)
{
}

void MTCommandList::Reset()
{
}

void MTCommandList::Close()
{
}

void MTCommandList::BindPipeline(const std::shared_ptr<Pipeline>& state)
{
}

void MTCommandList::BindBindingSet(const std::shared_ptr<BindingSet>& binding_set)
{
}

void MTCommandList::BeginRenderPass(const std::shared_ptr<RenderPass>& render_pass, const std::shared_ptr<Framebuffer>& framebuffer, const ClearDesc& clear_desc)
{
}

void MTCommandList::EndRenderPass()
{
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

void MTCommandList::DrawIndexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance)
{
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
}

void MTCommandList::SetScissorRect(int32_t left, int32_t top, uint32_t right, uint32_t bottom)
{
}

void MTCommandList::IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format)
{
}

void MTCommandList::IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource)
{
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
