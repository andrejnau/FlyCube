#pragma once
#include <RenderCommandList/RenderCommandList.h>
#include <Device/Device.h>
#include <ObjectCache/ObjectCache.h>

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
    RenderCommandListImpl(Device& device, ObjectCache& object_cache, CommandListType type);
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
    void Draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) override;
    void DrawIndexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance) override;
    void DrawIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset) override;
    void DrawIndexedIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset) override;
    void DrawIndirectCount(
        const std::shared_ptr<Resource>& argument_buffer,
        uint64_t argument_buffer_offset,
        const std::shared_ptr<Resource>& count_buffer,
        uint64_t count_buffer_offset,
        uint32_t max_draw_count,
        uint32_t stride) override;
    void DrawIndexedIndirectCount(
        const std::shared_ptr<Resource>& argument_buffer,
        uint64_t argument_buffer_offset,
        const std::shared_ptr<Resource>& count_buffer,
        uint64_t count_buffer_offset,
        uint32_t max_draw_count,
        uint32_t stride) override;
    void Dispatch(uint32_t thread_group_count_x, uint32_t thread_group_count_y, uint32_t thread_group_count_z) override;
    void DispatchIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset) override;
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
    void SetBinding(const BindKey& bind_key, const std::shared_ptr<View>& view);
    void UpdateDefaultSubresource(const std::shared_ptr<Resource>& resource, uint32_t subresource, const void* data, uint32_t row_pitch, uint32_t depth_pitch);
    void Apply();
    void ApplyPipeline();
    void ApplyBindingSet();
    ResourceStateTracker& GetResourceStateTracker(const std::shared_ptr<Resource>& resource);
    void CreateShaderTable(std::shared_ptr<Pipeline> pipeline);

    Device& m_device;
    ObjectCache& m_object_cache;
    std::vector<std::shared_ptr<Framebuffer>> m_framebuffers;
    std::shared_ptr<CommandList> m_command_list;
    uint32_t m_viewport_width = 0;
    uint32_t m_viewport_height = 0;

    std::map<BindKey, std::shared_ptr<View>> m_bound_resources;
    std::map<BindKey, std::shared_ptr<DeferredView>> m_bound_deferred_view;
    std::vector<std::shared_ptr<ResourceLazyViewDesc>> m_resource_lazy_view_descs;

    std::shared_ptr<Program> m_program;
    std::vector<std::shared_ptr<BindingSet>> m_binding_sets;

    ComputePipelineDesc m_compute_pipeline_desc = {};
    RayTracingPipelineDesc m_ray_tracing_pipeline_desc = {};
    GraphicsPipelineDesc m_graphic_pipeline_desc = {};

    std::vector<std::shared_ptr<Resource>> m_cmd_resources;

    std::map<std::shared_ptr<Resource>, ResourceStateTracker> m_resource_state_tracker;
    std::vector<ResourceBarrierDesc> m_lazy_barriers;
    uint64_t m_fence_value = 0;
    std::shared_ptr<BindingSetLayout> m_layout;

    RayTracingShaderTables m_shader_tables = {};
    std::shared_ptr<Resource> m_shader_table;
    PipelineType m_pipeline_type = PipelineType::kGraphics;
    std::shared_ptr<Pipeline> m_pipeline;
    std::shared_ptr<View> m_shading_rate_image;
    ShadingRateCombiner m_shading_rate_combiner = ShadingRateCombiner::kPassthrough;
};
