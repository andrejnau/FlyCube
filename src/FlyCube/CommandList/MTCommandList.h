#pragma once
#include "CommandList/CommandList.h"

#import <Metal/Metal.h>

#include <deque>
#include <functional>
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
    void DispatchMesh(uint32_t thread_group_count_x) override;
    void DispatchRays(const RayTracingShaderTables& shader_tables,
                      uint32_t width,
                      uint32_t height,
                      uint32_t depth) override;
    void ResourceBarrier(const std::vector<ResourceBarrierDesc>& barriers) override;
    void UAVResourceBarrier(const std::shared_ptr<Resource>& resource) override;
    void SetViewport(float x, float y, float width, float height) override;
    void SetScissorRect(int32_t left, int32_t top, uint32_t right, uint32_t bottom) override;
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

    id<MTLCommandBuffer> GetCommandBuffer();
    void OnSubmit();

private:
    id<MTLBuffer> PatchInstanceData(const std::shared_ptr<Resource>& instance_data,
                                    uint64_t instance_offset,
                                    uint32_t instance_count);
    void ApplyState();
    void ApplyBindingSet();
    void ApplyAndRecord(std::function<void()>&& cmd);

    MTDevice& m_device;
    id<MTLCommandBuffer> m_command_buffer = nullptr;
    id<MTLRenderCommandEncoder> m_render_encoder = nullptr;
    std::shared_ptr<Resource> m_index_buffer;
    gli::format m_index_format = gli::FORMAT_UNDEFINED;
    MTLViewport m_viewport = {};
    std::map<uint32_t, id<MTLBuffer>> m_vertices;
    std::shared_ptr<Pipeline> m_state;
    std::weak_ptr<Pipeline> m_last_state;
    std::shared_ptr<MTBindingSet> m_binding_set;
    std::weak_ptr<MTBindingSet> m_last_binding_set;
    std::deque<std::function<void()>> m_recorded_cmds;
    bool m_executed = false;
};
