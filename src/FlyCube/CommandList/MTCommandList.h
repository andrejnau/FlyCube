#pragma once
#include "CommandList/CommandList.h"

#import <Metal/Metal.h>

#include <map>

class MTDevice;
class MTGraphicsPipeline;
class MTBindingSet;

class MTCommandList : public CommandList {
public:
    MTCommandList(MTDevice& device, CommandListType type);
    void Reset() override;
    void Close() override;
    void BindPipeline(const std::shared_ptr<Pipeline>& state) override;
    void BindBindingSet(const std::shared_ptr<BindingSet>& binding_set) override;
    void BeginRenderPass(const std::shared_ptr<RenderPass>& render_pass,
                         const std::shared_ptr<Framebuffer>& framebuffer,
                         const ClearDesc& clear_desc) override;
    void EndRenderPass() override;
    void BeginEvent(const std::string& name) override;
    void EndEvent() override;
    void Draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) override;
    void DrawIndexed(uint32_t index_count,
                     uint32_t instance_count,
                     uint32_t first_index,
                     int32_t vertex_offset,
                     uint32_t first_instance) override;
    void DrawIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset) override;
    void DrawIndexedIndirect(const std::shared_ptr<Resource>& argument_buffer,
                             uint64_t argument_buffer_offset) override;
    void DrawIndirectCount(const std::shared_ptr<Resource>& argument_buffer,
                           uint64_t argument_buffer_offset,
                           const std::shared_ptr<Resource>& count_buffer,
                           uint64_t count_buffer_offset,
                           uint32_t max_draw_count,
                           uint32_t stride) override;
    void DrawIndexedIndirectCount(const std::shared_ptr<Resource>& argument_buffer,
                                  uint64_t argument_buffer_offset,
                                  const std::shared_ptr<Resource>& count_buffer,
                                  uint64_t count_buffer_offset,
                                  uint32_t max_draw_count,
                                  uint32_t stride) override;
    void Dispatch(uint32_t thread_group_count_x, uint32_t thread_group_count_y, uint32_t thread_group_count_z) override;
    void DispatchIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset) override;
    void DispatchMesh(uint32_t thread_group_count_x,
                      uint32_t thread_group_count_y,
                      uint32_t thread_group_count_z) override;
    void DispatchRays(const RayTracingShaderTables& shader_tables,
                      uint32_t width,
                      uint32_t height,
                      uint32_t depth) override;
    void ResourceBarrier(const std::vector<ResourceBarrierDesc>& barriers) override;
    void UAVResourceBarrier(const std::shared_ptr<Resource>& resource) override;
    void SetViewport(float x, float y, float width, float height) override;
    void SetScissorRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom) override;
    void IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format) override;
    void IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource) override;
    void RSSetShadingRate(ShadingRate shading_rate, const std::array<ShadingRateCombiner, 2>& combiners) override;
    void BuildBottomLevelAS(const std::shared_ptr<Resource>& src,
                            const std::shared_ptr<Resource>& dst,
                            const std::shared_ptr<Resource>& scratch,
                            uint64_t scratch_offset,
                            const std::vector<RaytracingGeometryDesc>& descs,
                            BuildAccelerationStructureFlags flags) override;
    void BuildTopLevelAS(const std::shared_ptr<Resource>& src,
                         const std::shared_ptr<Resource>& dst,
                         const std::shared_ptr<Resource>& scratch,
                         uint64_t scratch_offset,
                         const std::shared_ptr<Resource>& instance_data,
                         uint64_t instance_offset,
                         uint32_t instance_count,
                         BuildAccelerationStructureFlags flags) override;
    void CopyAccelerationStructure(const std::shared_ptr<Resource>& src,
                                   const std::shared_ptr<Resource>& dst,
                                   CopyAccelerationStructureMode mode) override;
    void CopyBuffer(const std::shared_ptr<Resource>& src_buffer,
                    const std::shared_ptr<Resource>& dst_buffer,
                    const std::vector<BufferCopyRegion>& regions) override;
    void CopyBufferToTexture(const std::shared_ptr<Resource>& src_buffer,
                             const std::shared_ptr<Resource>& dst_texture,
                             const std::vector<BufferToTextureCopyRegion>& regions) override;
    void CopyTexture(const std::shared_ptr<Resource>& src_texture,
                     const std::shared_ptr<Resource>& dst_texture,
                     const std::vector<TextureCopyRegion>& regions) override;
    void WriteAccelerationStructuresProperties(const std::vector<std::shared_ptr<Resource>>& acceleration_structures,
                                               const std::shared_ptr<QueryHeap>& query_heap,
                                               uint32_t first_query) override;
    void ResolveQueryData(const std::shared_ptr<QueryHeap>& query_heap,
                          uint32_t first_query,
                          uint32_t query_count,
                          const std::shared_ptr<Resource>& dst_buffer,
                          uint64_t dst_offset) override;
    void SetName(const std::string& name) override;

    id<MTL4CommandBuffer> GetCommandBuffer();

private:
    MTL4BufferRange PatchInstanceData(const std::shared_ptr<Resource>& instance_data,
                                      uint64_t instance_offset,
                                      uint32_t instance_count);
    void ApplyComputeState();
    void ApplyGraphicsState();
    void AddGraphicsBarriers();
    void AddComputeBarriers();
    void CreateArgumentTables();
    void AddAllocation(id<MTLAllocation> allocation);
    void OpenComputeEncoder();
    void CloseComputeEncoder();

    MTDevice& m_device;
    id<MTL4CommandBuffer> m_command_buffer = nullptr;
    id<MTL4CommandAllocator> m_allocator = nullptr;
    id<MTL4RenderCommandEncoder> m_render_encoder = nullptr;
    id<MTL4ComputeCommandEncoder> m_compute_encoder = nullptr;
    id<MTLBuffer> m_index_buffer = nullptr;
    gli::format m_index_format = gli::FORMAT_UNDEFINED;
    MTLViewport m_viewport = {};
    MTLScissorRect m_scissor = {};
    std::shared_ptr<Pipeline> m_state;
    std::shared_ptr<MTBindingSet> m_binding_set;
    std::map<ShaderType, id<MTL4ArgumentTable>> m_argument_tables;
    id<MTLResidencySet> m_residency_set = nullptr;
    bool m_need_apply_state = false;
    bool m_need_apply_binding_set = false;
    MTLStages m_render_barrier_after_stages = 0;
    MTLStages m_render_barrier_before_stages = 0;
    MTLStages m_compute_barrier_after_stages = 0;
    MTLStages m_compute_barrier_before_stages = 0;
    bool m_closed = false;
};
