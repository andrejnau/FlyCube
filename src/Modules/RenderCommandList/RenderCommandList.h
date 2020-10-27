#pragma once
#include <Instance/QueryInterface.h>
#include <Resource/Resource.h>
#include <Instance/BaseTypes.h>

class RenderCommandList : public QueryInterface
{
public:
    virtual ~RenderCommandList() = default;
    virtual void Reset() = 0;
    virtual void Close() = 0;
    virtual void Attach(const BindKey& bind_key, const std::shared_ptr<Resource>& resource = {}, const LazyViewDesc& view_desc = {}) = 0;
    virtual void Attach(const BindKey& bind_key, const std::shared_ptr<DeferredView>& view) = 0;
    virtual void Attach(const BindKey& bind_key, const std::shared_ptr<View>& view) = 0;
    virtual void SetRasterizeState(const RasterizerDesc& desc) = 0;
    virtual void SetBlendState(const BlendDesc& desc) = 0;
    virtual void SetDepthStencilState(const DepthStencilDesc& desc) = 0;
    virtual void UseProgram(const std::shared_ptr<Program>& program) = 0;
    virtual void BeginRenderPass(const RenderPassBeginDesc& desc) = 0;
    virtual void EndRenderPass() = 0;
    virtual void BeginEvent(const std::string& name) = 0;
    virtual void EndEvent() = 0;
    virtual void DrawIndexed(uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location) = 0;
    virtual void Dispatch(uint32_t thread_group_count_x, uint32_t thread_group_count_y, uint32_t thread_group_count_z) = 0;
    virtual void DispatchMesh(uint32_t thread_group_count_x) = 0;
    virtual void DispatchRays(uint32_t width, uint32_t height, uint32_t depth) = 0;
    virtual void SetViewport(float x, float y, float width, float height) = 0;
    virtual void SetScissorRect(int32_t left, int32_t top, uint32_t right, uint32_t bottom) = 0;
    virtual void IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format) = 0;
    virtual void IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource) = 0;
    virtual void RSSetShadingRate(ShadingRate shading_rate, const std::array<ShadingRateCombiner, 2>& combiners = {}) = 0;
    virtual void RSSetShadingRateImage(const std::shared_ptr<Resource>& resource) = 0;
    virtual void BuildBottomLevelAS(const std::shared_ptr<Resource>& src, const std::shared_ptr<Resource>& dst, const std::vector<RaytracingGeometryDesc>& descs, BuildAccelerationStructureFlags flags = BuildAccelerationStructureFlags::kNone) = 0;
    virtual void BuildTopLevelAS(const std::shared_ptr<Resource>& src, const std::shared_ptr<Resource>& dst, const std::vector<std::pair<std::shared_ptr<Resource>, glm::mat4>>& geometry, BuildAccelerationStructureFlags flags = BuildAccelerationStructureFlags::kNone) = 0;
    virtual void CopyAccelerationStructure(const std::shared_ptr<Resource>& src, const std::shared_ptr<Resource>& dst, CopyAccelerationStructureMode mode) = 0;
    virtual void CopyBuffer(const std::shared_ptr<Resource>& src_buffer, const std::shared_ptr<Resource>& dst_buffer,
                            const std::vector<BufferCopyRegion>& regions) = 0;
    virtual void CopyBufferToTexture(const std::shared_ptr<Resource>& src_buffer, const std::shared_ptr<Resource>& dst_texture,
                                     const std::vector<BufferToTextureCopyRegion>& regions) = 0;
    virtual void CopyTexture(const std::shared_ptr<Resource>& src_texture, const std::shared_ptr<Resource>& dst_texture,
                             const std::vector<TextureCopyRegion>& regions) = 0;
    virtual void UpdateSubresource(const std::shared_ptr<Resource>& resource, uint32_t subresource, const void* data, uint32_t row_pitch = 0, uint32_t depth_pitch = 0) = 0;
    virtual uint64_t& GetFenceValue() = 0;
};
