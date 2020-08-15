#pragma once
#include "CommandListBox/CommandListBoxBase.h"
#include <Instance/BaseTypes.h>
#include <Instance/Instance.h>
#include <AppBox/Settings.h>
#include <memory>
#include <ApiType/ApiType.h>

#include <GLFW/glfw3.h>
#include <memory>
#include <array>
#include <set>

#include <Resource/Resource.h>
#include <glm/glm.hpp>
#include <gli/gli.hpp>

class CommandListBox : public CommandListBoxBase
{
public:
    CommandListBox(Device& device);

    std::shared_ptr<CommandList>& GetCommandList()
    {
        return m_command_list;
    }

    void Reset();
    void Close();

    void EndRenderPass()
    {
        if (m_is_open_render_pass)
        {
            m_command_list->EndRenderPass();
            m_is_open_render_pass = false;
        }
    }

    void Attach(const BindKey& bind_key, const std::shared_ptr<DeferredView>& view);
    void Attach(const BindKey& bind_key, const std::shared_ptr<Resource>& resource = {}, const LazyViewDesc& view_desc = {});
    void SetRasterizeState(const RasterizerDesc& desc);
    void SetBlendState(const BlendDesc& desc);
    void SetDepthStencilState(const DepthStencilDesc& desc);
    void UseProgram(std::shared_ptr<Program>& program);
    void BeginEvent(const std::string& name);
    void EndEvent();
    void ClearColor(const BindKey& bind_key, const glm::vec4& color);
    void ClearDepth(const BindKey& bind_key, float depth);
    void DrawIndexed(uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location);
    void Dispatch(uint32_t thread_group_count_x, uint32_t thread_group_count_y, uint32_t thread_group_count_z);
    void DispatchRays(uint32_t width, uint32_t height, uint32_t depth);
    void SetViewport(float width, float height);
    void SetScissorRect(int32_t left, int32_t top, uint32_t right, uint32_t bottom);
    void IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format);
    void IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource);
    void RSSetShadingRate(ShadingRate shading_rate, const std::array<ShadingRateCombiner, 2>& combiners = {});
    void RSSetShadingRateImage(const std::shared_ptr<Resource>& resource);
    void CopyTexture(const std::shared_ptr<Resource>& src_texture, const std::shared_ptr<Resource>& dst_texture, const std::vector<TextureCopyRegion>& regions);
    void UpdateSubresource(const std::shared_ptr<Resource>& resource, uint32_t subresource, const void* data, uint32_t row_pitch = 0, uint32_t depth_pitch = 0);
    void BufferBarrier(const std::shared_ptr<Resource>& resource, ResourceState state);
    void ViewBarrier(const std::shared_ptr<View>& view, ResourceState state);
    void ImageBarrier(const std::shared_ptr<Resource>& resource, uint32_t base_mip_level, uint32_t level_count, uint32_t base_array_layer, uint32_t layer_count, ResourceState state);

    std::shared_ptr<Resource> CreateBottomLevelAS(const std::shared_ptr<Resource>& src, const std::vector<RaytracingGeometryDesc>& descs, BuildAccelerationStructureFlags flags = BuildAccelerationStructureFlags::kNone);
    std::shared_ptr<Resource> CreateTopLevelAS(const std::shared_ptr<Resource>& src, const std::vector<std::pair<std::shared_ptr<Resource>, glm::mat4>>& geometry, BuildAccelerationStructureFlags flags = BuildAccelerationStructureFlags::kNone);
    void CopyAccelerationStructure(const std::shared_ptr<Resource>& src, const std::shared_ptr<Resource>& dst, CopyAccelerationStructureMode mode);

    void ReleaseRequest(const std::shared_ptr<Resource>& resource);

    uint64_t fence_value = 0;

private:
    void ApplyBindings();

    Device& m_device;
    std::shared_ptr<Fence> m_fence;
    bool m_is_open_render_pass = false;
    uint32_t m_viewport_width = 0;
    uint32_t m_viewport_height = 0;

private:
    void Attach(const BindKey& bind_key, const std::shared_ptr<View>& view);
    void OnAttachSRV(const BindKey& bind_key, const std::shared_ptr<View>& view);
    void OnAttachUAV(const BindKey& bind_key, const std::shared_ptr<View>& view);
    void OnAttachRTV(const BindKey& bind_key, const std::shared_ptr<View>& view);
    void OnAttachDSV(const BindKey& bind_key, const std::shared_ptr<View>& view);

    std::shared_ptr<View> CreateView(const BindKey& bind_key, const std::shared_ptr<Resource>& resource, const LazyViewDesc& view_desc);
    std::shared_ptr<View> FindOmView(ShaderType shader_type, ViewType view_type, uint32_t slot);

    void SetBinding(const BindKey& bind_key, const std::shared_ptr<View>& view);
    void SetBindingImpl(const BindKey& bind_key, const std::shared_ptr<View>& view, std::map<BindKey, std::shared_ptr<View>>& bound_resources);

    std::map<BindKey, std::shared_ptr<View>> m_bound_resources;
    std::map<BindKey, std::shared_ptr<View>> m_bound_om_resources;
    std::map<BindKey, std::shared_ptr<DeferredView>> m_bound_deferred_view;
    std::vector<std::shared_ptr<ResourceLazyViewDesc>> m_resource_lazy_view_descs;

    std::vector<std::shared_ptr<Shader>> m_shaders;
    std::shared_ptr<Program> m_program;
    std::shared_ptr<Pipeline> m_pipeline;
    std::shared_ptr<BindingSet> m_binding_set;
    std::vector<std::shared_ptr<BindingSet>> m_binding_sets;

    std::map<GraphicsPipelineDesc, std::shared_ptr<Pipeline>> m_pso;
    std::map<ComputePipelineDesc, std::shared_ptr<Pipeline>> m_compute_pso;
    std::map<std::tuple<uint32_t, uint32_t, std::vector<std::shared_ptr<View>>, std::shared_ptr<View>>, std::shared_ptr<Framebuffer>> m_framebuffers;
    std::shared_ptr<RenderPass> m_render_pass;
    std::shared_ptr<Framebuffer> m_framebuffer;
    std::map<std::tuple<BindKey, std::shared_ptr<Resource>, LazyViewDesc>, std::shared_ptr<View>> m_views;
    ComputePipelineDesc m_compute_pipeline_desc = {};
    GraphicsPipelineDesc m_graphic_pipeline_desc = {};
    std::map<RenderPassDesc, std::shared_ptr<RenderPass>> m_render_pass_cache;
    RenderPassDesc m_render_pass_desc = {};

    void UpdateSubresourceDefault(const std::shared_ptr<Resource>& resource, uint32_t subresource, const void* data, uint32_t row_pitch, uint32_t depth_pitch);
    std::vector<std::shared_ptr<Resource>> m_cmd_resources;
};
