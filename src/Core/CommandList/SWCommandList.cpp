#include "CommandList/SWCommandList.h"
#include <Device/VKDevice.h>

SWCommandList::SWCommandList(SWDevice& device, CommandListType type)
    : m_device(device)
{
}

void SWCommandList::Reset()
{
}

void SWCommandList::Close()
{
}

void SWCommandList::BindPipeline(const std::shared_ptr<Pipeline>& state)
{
}

void SWCommandList::BindBindingSet(const std::shared_ptr<BindingSet>& binding_set)
{
}

void SWCommandList::BeginRenderPass(const std::shared_ptr<RenderPass>& render_pass, const std::shared_ptr<Framebuffer>& framebuffer, const ClearDesc& clear_desc)
{
}

void SWCommandList::EndRenderPass()
{
}

void SWCommandList::BeginEvent(const std::string& name)
{
}

void SWCommandList::EndEvent()
{
}

void SWCommandList::Draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
{
}

void SWCommandList::DrawIndexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance)
{
}

void SWCommandList::DrawIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset)
{
}

void SWCommandList::DrawIndexedIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset)
{
}

void SWCommandList::DrawIndirectCount(
    const std::shared_ptr<Resource>& argument_buffer,
    uint64_t argument_buffer_offset,
    const std::shared_ptr<Resource>& count_buffer,
    uint64_t count_buffer_offset,
    uint32_t max_draw_count,
    uint32_t stride)
{
}

void SWCommandList::DrawIndexedIndirectCount(
    const std::shared_ptr<Resource>& argument_buffer,
    uint64_t argument_buffer_offset,
    const std::shared_ptr<Resource>& count_buffer,
    uint64_t count_buffer_offset,
    uint32_t max_draw_count,
    uint32_t stride)
{
}

void SWCommandList::Dispatch(uint32_t thread_group_count_x, uint32_t thread_group_count_y, uint32_t thread_group_count_z)
{
}

void SWCommandList::DispatchIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset)
{
}

void SWCommandList::DispatchMesh(uint32_t thread_group_count_x)
{
}

void SWCommandList::DispatchRays(const RayTracingShaderTables& shader_tables, uint32_t width, uint32_t height, uint32_t depth)
{
}

void SWCommandList::ResourceBarrier(const std::vector<ResourceBarrierDesc>& barriers)
{
}

void SWCommandList::UAVResourceBarrier(const std::shared_ptr<Resource>& /*resource*/)
{
}

void SWCommandList::SetViewport(float x, float y, float width, float height)
{
}

void SWCommandList::SetScissorRect(int32_t left, int32_t top, uint32_t right, uint32_t bottom)
{
}

void SWCommandList::IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format)
{
}

void SWCommandList::IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource)
{
}

void SWCommandList::RSSetShadingRate(ShadingRate shading_rate, const std::array<ShadingRateCombiner, 2>& combiners)
{
}

void SWCommandList::BuildBottomLevelAS(
    const std::shared_ptr<Resource>& src,
    const std::shared_ptr<Resource>& dst,
    const std::shared_ptr<Resource>& scratch,
    uint64_t scratch_offset,
    const std::vector<RaytracingGeometryDesc>& descs,
    BuildAccelerationStructureFlags flags)
{
}

void SWCommandList::BuildTopLevelAS(
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

void SWCommandList::CopyAccelerationStructure(const std::shared_ptr<Resource>& src, const std::shared_ptr<Resource>& dst, CopyAccelerationStructureMode mode)
{
}

void SWCommandList::CopyBuffer(const std::shared_ptr<Resource>& src_buffer, const std::shared_ptr<Resource>& dst_buffer,
                               const std::vector<BufferCopyRegion>& regions)
{
}

void SWCommandList::CopyBufferToTexture(const std::shared_ptr<Resource>& src_buffer, const std::shared_ptr<Resource>& dst_texture,
                                        const std::vector<BufferToTextureCopyRegion>& regions)
{
}

void SWCommandList::CopyTexture(const std::shared_ptr<Resource>& src_texture, const std::shared_ptr<Resource>& dst_texture,
                                const std::vector<TextureCopyRegion>& regions)
{
}

void SWCommandList::WriteAccelerationStructuresProperties(
    const std::vector<std::shared_ptr<Resource>>& acceleration_structures,
    const std::shared_ptr<QueryHeap>& query_heap,
    uint32_t first_query)
{
}

void SWCommandList::ResolveQueryData(
    const std::shared_ptr<QueryHeap>& query_heap,
    uint32_t first_query,
    uint32_t query_count,
    const std::shared_ptr<Resource>& dst_buffer,
    uint64_t dst_offset)
{
}
