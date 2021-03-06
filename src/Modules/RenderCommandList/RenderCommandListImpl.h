#pragma once
#include <RenderCommandList/RenderCommandList.h>
#include <Device/Device.h>

struct LazyResourceBarrierDesc
{
    std::shared_ptr<Resource> resource;
    ResourceState state;
    uint32_t base_mip_level = 0;
    uint32_t level_count = 1;
    uint32_t base_array_layer = 0;
    uint32_t layer_count = 1;
};

constexpr bool kUseFakeClose = true;

class RenderCommandListImpl : public RenderCommandList
{
public:
    RenderCommandListImpl(Device& device, CommandListType type);
    const std::shared_ptr<CommandList>& GetCommandList();
    void LazyResourceBarrier(const std::vector<LazyResourceBarrierDesc>& barriers);
    const std::map<std::shared_ptr<Resource>, ResourceStateTracker>& GetResourceStateTrackers() const;
    const std::vector<ResourceBarrierDesc>& GetLazyBarriers() const;

    void Reset() override;
    void Close() override;
    void Attach(const BindKey& bind_key, const std::shared_ptr<Resource>& resource = {}, const LazyViewDesc& view_desc = {}) override;
    void Attach(const BindKey& bind_key, const std::shared_ptr<DeferredView>& view) override;
    void Attach(const BindKey& bind_key, const std::shared_ptr<View>& view) override;
    void SetRasterizeState(const RasterizerDesc& desc) override;
    void SetBlendState(const BlendDesc& desc) override;
    void SetDepthStencilState(const DepthStencilDesc& desc) override;
    void UseProgram(const std::shared_ptr<Program>& program) override;
    void BeginRenderPass(const RenderPassBeginDesc& desc) override;
    void EndRenderPass() override;
    void BeginEvent(const std::string& name) override;
    void EndEvent() override;
    void DrawIndexed(uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location) override;
    void Dispatch(uint32_t thread_group_count_x, uint32_t thread_group_count_y, uint32_t thread_group_count_z) override;
    void DispatchMesh(uint32_t thread_group_count_x) override;
    void DispatchRays(uint32_t width, uint32_t height, uint32_t depth) override;
    void SetViewport(float x, float y, float width, float height) override;
    void SetScissorRect(int32_t left, int32_t top, uint32_t right, uint32_t bottom) override;
    void IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format) override;
    void IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource) override;
    void RSSetShadingRateImage(const std::shared_ptr<View>& view) override;
    void BuildBottomLevelAS(const std::shared_ptr<Resource>& src, const std::shared_ptr<Resource>& dst, const std::vector<RaytracingGeometryDesc>& descs, BuildAccelerationStructureFlags flags = BuildAccelerationStructureFlags::kNone) override;
    void BuildTopLevelAS(const std::shared_ptr<Resource>& src, const std::shared_ptr<Resource>& dst, const std::vector<std::pair<std::shared_ptr<Resource>, glm::mat4>>& geometry, BuildAccelerationStructureFlags flags = BuildAccelerationStructureFlags::kNone) override;
    void CopyAccelerationStructure(const std::shared_ptr<Resource>& src, const std::shared_ptr<Resource>& dst, CopyAccelerationStructureMode mode) override;
    void CopyBuffer(const std::shared_ptr<Resource>& src_buffer, const std::shared_ptr<Resource>& dst_buffer,
                            const std::vector<BufferCopyRegion>& regions) override;
    void CopyBufferToTexture(const std::shared_ptr<Resource>& src_buffer, const std::shared_ptr<Resource>& dst_texture,
                                     const std::vector<BufferToTextureCopyRegion>& regions) override;
    void CopyTexture(const std::shared_ptr<Resource>& src_texture, const std::shared_ptr<Resource>& dst_texture,
                             const std::vector<TextureCopyRegion>& regions) override;
    void UpdateSubresource(const std::shared_ptr<Resource>& resource, uint32_t subresource, const void* data, uint32_t row_pitch = 0, uint32_t depth_pitch = 0) override;
    uint64_t& GetFenceValue() override;

private:
    void BufferBarrier(const std::shared_ptr<Resource>& resource, ResourceState state);
    void ViewBarrier(const std::shared_ptr<View>& view, ResourceState state);
    void ImageBarrier(const std::shared_ptr<Resource>& resource, uint32_t base_mip_level, uint32_t level_count, uint32_t base_array_layer, uint32_t layer_count, ResourceState state);
    void OnAttachSRV(const BindKey& bind_key, const std::shared_ptr<View>& view);
    void OnAttachUAV(const BindKey& bind_key, const std::shared_ptr<View>& view);
    std::shared_ptr<View> CreateView(const BindKey& bind_key, const std::shared_ptr<Resource>& resource, const LazyViewDesc& view_desc);
    void SetBinding(const BindKey& bind_key, const std::shared_ptr<View>& view);
    void UpdateSubresourceDefault(const std::shared_ptr<Resource>& resource, uint32_t subresource, const void* data, uint32_t row_pitch, uint32_t depth_pitch);
    void ApplyPipeline();
    void ApplyBindingSet();
    ResourceStateTracker& GetResourceStateTracker(const std::shared_ptr<Resource>& resource);

    Device& m_device;
    std::shared_ptr<CommandList> m_command_list;
    uint32_t m_viewport_width = 0;
    uint32_t m_viewport_height = 0;

    std::map<BindKey, std::shared_ptr<View>> m_bound_resources;
    std::map<BindKey, std::shared_ptr<DeferredView>> m_bound_deferred_view;
    std::vector<std::shared_ptr<ResourceLazyViewDesc>> m_resource_lazy_view_descs;

    std::shared_ptr<Program> m_program;
    std::vector<std::shared_ptr<BindingSet>> m_binding_sets;

    std::map<GraphicsPipelineDesc, std::shared_ptr<Pipeline>> m_graphics_pipeline_cache;
    std::map<ComputePipelineDesc, std::shared_ptr<Pipeline>> m_compute_pipeline_cache;
    std::map<RayTracingPipelineDesc, std::shared_ptr<Pipeline>> m_ray_tracing_pipeline_cache;
    std::map<std::tuple<uint32_t, uint32_t, std::vector<std::shared_ptr<View>>, std::shared_ptr<View>>, std::shared_ptr<Framebuffer>> m_framebuffers;
    std::map<std::tuple<BindKey, std::shared_ptr<Resource>, LazyViewDesc>, std::shared_ptr<View>> m_views;
    ComputePipelineDesc m_compute_pipeline_desc = {};
    RayTracingPipelineDesc m_ray_tracing_pipeline_desc = {};
    GraphicsPipelineDesc m_graphic_pipeline_desc = {};
    std::map<RenderPassDesc, std::shared_ptr<RenderPass>> m_render_pass_cache;

    std::vector<std::shared_ptr<Resource>> m_cmd_resources;

    std::map<std::shared_ptr<Resource>, ResourceStateTracker> m_resource_state_tracker;
    std::vector<ResourceBarrierDesc> m_lazy_barriers;
    uint64_t m_fence_value = 0;
    std::shared_ptr<BindingSetLayout> m_layout;
    std::map<std::vector<BindKey>, std::shared_ptr<BindingSetLayout>> m_layout_cache;
    std::map<std::pair<std::shared_ptr<BindingSetLayout>, std::vector<BindingDesc>>, std::shared_ptr<BindingSet>> m_binding_set_cache;
};
