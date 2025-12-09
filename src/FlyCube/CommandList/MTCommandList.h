#pragma once
#include "CommandList/CommandList.h"

#import <Metal/Metal.h>

#include <map>
#include <optional>

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
    void BeginRenderPass(const RenderPassDesc& render_pass_desc) override;
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
    void SetViewport(float x, float y, float width, float height, float min_depth, float max_depth) override;
    void SetScissorRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom) override;
    void IASetIndexBuffer(const std::shared_ptr<Resource>& resource, uint64_t offset, gli::format format) override;
    void IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource, uint64_t offset) override;
    void RSSetShadingRate(ShadingRate shading_rate, const std::array<ShadingRateCombiner, 2>& combiners) override;
    void SetDepthBounds(float min_depth_bounds, float max_depth_bounds) override;
    void SetStencilReference(uint32_t stencil_reference) override;
    void SetBlendConstants(float red, float green, float blue, float alpha) override;
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
                             const std::vector<BufferTextureCopyRegion>& regions) override;
    void CopyTextureToBuffer(const std::shared_ptr<Resource>& src_texture,
                             const std::shared_ptr<Resource>& dst_buffer,
                             const std::vector<BufferTextureCopyRegion>& regions) override;
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
    void CopyBufferTextureImpl(bool buffer_src,
                               const std::shared_ptr<Resource>& buffer,
                               const std::shared_ptr<Resource>& texture,
                               const std::vector<BufferTextureCopyRegion>& regions);
    id<MTLBuffer> PatchInstanceData(const std::shared_ptr<Resource>& instance_data,
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

    MTDevice& device_;
    id<MTL4CommandBuffer> command_buffer_ = nullptr;
    id<MTL4CommandAllocator> allocator_ = nullptr;
    id<MTL4RenderCommandEncoder> render_encoder_ = nullptr;
    id<MTL4ComputeCommandEncoder> compute_encoder_ = nullptr;
    id<MTLBuffer> index_buffer_ = nullptr;
    uint64_t index_buffer_offset_ = 0;
    gli::format index_format_ = gli::FORMAT_UNDEFINED;
    MTLViewport viewport_ = {};
    MTLScissorRect scissor_ = {};
    float min_depth_bounds_ = 0.0;
    float max_depth_bounds_ = 1.0;
    uint32_t stencil_reference_ = 0;
    std::optional<std::array<float, 4>> blend_constants_;
    std::shared_ptr<Pipeline> state_;
    std::shared_ptr<MTBindingSet> binding_set_;
    std::map<ShaderType, id<MTL4ArgumentTable>> argument_tables_;
    id<MTLResidencySet> residency_set_ = nullptr;
    std::vector<id<MTLBuffer>> patch_buffers_;
    bool need_apply_state_ = false;
    bool need_apply_binding_set_ = false;
    MTLStages render_barrier_after_stages_ = 0;
    MTLStages render_barrier_before_stages_ = 0;
    MTLStages compute_barrier_after_stages_ = 0;
    MTLStages compute_barrier_before_stages_ = 0;
    bool closed_ = false;
};
