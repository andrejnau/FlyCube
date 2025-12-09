#pragma once
#include "BindingSet/BindingSet.h"
#include "Instance/BaseTypes.h"
#include "Instance/QueryInterface.h"
#include "Pipeline/Pipeline.h"
#include "QueryHeap/QueryHeap.h"
#include "Resource/Resource.h"
#include "View/View.h"

#include <gli/format.hpp>

#include <array>
#include <memory>

class CommandList : public QueryInterface {
public:
    virtual ~CommandList() = default;
    virtual void Reset() = 0;
    virtual void Close() = 0;
    virtual void BindPipeline(const std::shared_ptr<Pipeline>& state) = 0;
    virtual void BindBindingSet(const std::shared_ptr<BindingSet>& binding_set) = 0;
    virtual void BeginRenderPass(const RenderPassDesc& render_pass_desc) = 0;
    virtual void EndRenderPass() = 0;
    virtual void BeginEvent(const std::string& name) = 0;
    virtual void EndEvent() = 0;
    virtual void Draw(uint32_t vertex_count,
                      uint32_t instance_count,
                      uint32_t first_vertex,
                      uint32_t first_instance) = 0;
    virtual void DrawIndexed(uint32_t index_count,
                             uint32_t instance_count,
                             uint32_t first_index,
                             int32_t vertex_offset,
                             uint32_t first_instance) = 0;
    virtual void DrawIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset) = 0;
    virtual void DrawIndexedIndirect(const std::shared_ptr<Resource>& argument_buffer,
                                     uint64_t argument_buffer_offset) = 0;
    virtual void DrawIndirectCount(const std::shared_ptr<Resource>& argument_buffer,
                                   uint64_t argument_buffer_offset,
                                   const std::shared_ptr<Resource>& count_buffer,
                                   uint64_t count_buffer_offset,
                                   uint32_t max_draw_count,
                                   uint32_t stride) = 0;
    virtual void DrawIndexedIndirectCount(const std::shared_ptr<Resource>& argument_buffer,
                                          uint64_t argument_buffer_offset,
                                          const std::shared_ptr<Resource>& count_buffer,
                                          uint64_t count_buffer_offset,
                                          uint32_t max_draw_count,
                                          uint32_t stride) = 0;
    virtual void Dispatch(uint32_t thread_group_count_x,
                          uint32_t thread_group_count_y,
                          uint32_t thread_group_count_z) = 0;
    virtual void DispatchIndirect(const std::shared_ptr<Resource>& argument_buffer,
                                  uint64_t argument_buffer_offset) = 0;
    virtual void DispatchMesh(uint32_t thread_group_count_x,
                              uint32_t thread_group_count_y,
                              uint32_t thread_group_count_z) = 0;
    virtual void DispatchRays(const RayTracingShaderTables& shader_tables,
                              uint32_t width,
                              uint32_t height,
                              uint32_t depth) = 0;
    virtual void ResourceBarrier(const std::vector<ResourceBarrierDesc>& barriers) = 0;
    virtual void UAVResourceBarrier(const std::shared_ptr<Resource>& resource) = 0;
    virtual void SetViewport(float x, float y, float width, float height, float min_depth, float max_depth) = 0;
    virtual void SetScissorRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom) = 0;
    virtual void IASetIndexBuffer(const std::shared_ptr<Resource>& resource, uint64_t offset, gli::format format) = 0;
    virtual void IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource, uint64_t offset) = 0;
    virtual void RSSetShadingRate(ShadingRate shading_rate, const std::array<ShadingRateCombiner, 2>& combiners) = 0;
    virtual void SetDepthBounds(float min_depth_bounds, float max_depth_bounds) = 0;
    virtual void SetStencilReference(uint32_t stencil_reference) = 0;
    virtual void SetBlendConstants(float red, float green, float blue, float alpha) = 0;
    virtual void BuildBottomLevelAS(const std::shared_ptr<Resource>& src,
                                    const std::shared_ptr<Resource>& dst,
                                    const std::shared_ptr<Resource>& scratch,
                                    uint64_t scratch_offset,
                                    const std::vector<RaytracingGeometryDesc>& descs,
                                    BuildAccelerationStructureFlags flags) = 0;
    virtual void BuildTopLevelAS(const std::shared_ptr<Resource>& src,
                                 const std::shared_ptr<Resource>& dst,
                                 const std::shared_ptr<Resource>& scratch,
                                 uint64_t scratch_offset,
                                 const std::shared_ptr<Resource>& instance_data,
                                 uint64_t instance_offset,
                                 uint32_t instance_count,
                                 BuildAccelerationStructureFlags flags) = 0;
    virtual void CopyAccelerationStructure(const std::shared_ptr<Resource>& src,
                                           const std::shared_ptr<Resource>& dst,
                                           CopyAccelerationStructureMode mode) = 0;
    virtual void CopyBuffer(const std::shared_ptr<Resource>& src_buffer,
                            const std::shared_ptr<Resource>& dst_buffer,
                            const std::vector<BufferCopyRegion>& regions) = 0;
    virtual void CopyBufferToTexture(const std::shared_ptr<Resource>& src_buffer,
                                     const std::shared_ptr<Resource>& dst_texture,
                                     const std::vector<BufferTextureCopyRegion>& regions) = 0;
    virtual void CopyTextureToBuffer(const std::shared_ptr<Resource>& src_texture,
                                     const std::shared_ptr<Resource>& dst_buffer,
                                     const std::vector<BufferTextureCopyRegion>& regions) = 0;
    virtual void CopyTexture(const std::shared_ptr<Resource>& src_texture,
                             const std::shared_ptr<Resource>& dst_texture,
                             const std::vector<TextureCopyRegion>& regions) = 0;
    virtual void WriteAccelerationStructuresProperties(
        const std::vector<std::shared_ptr<Resource>>& acceleration_structures,
        const std::shared_ptr<QueryHeap>& query_heap,
        uint32_t first_query) = 0;
    virtual void ResolveQueryData(const std::shared_ptr<QueryHeap>& query_heap,
                                  uint32_t first_query,
                                  uint32_t query_count,
                                  const std::shared_ptr<Resource>& dst_buffer,
                                  uint64_t dst_offset) = 0;
    virtual void SetName(const std::string& name) = 0;
};
