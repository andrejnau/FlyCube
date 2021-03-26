#pragma once
#include <Instance/BaseTypes.h>
#include <Pipeline/Pipeline.h>
#include <Device/Device.h>
#include <map>

class ObjectCache
{
public:
    ObjectCache(Device& device);

    std::shared_ptr<Pipeline> GetPipeline(const GraphicsPipelineDesc& desc);
    std::shared_ptr<Pipeline> GetPipeline(const ComputePipelineDesc& desc);
    std::shared_ptr<Pipeline> GetPipeline(const RayTracingPipelineDesc& desc);
    std::shared_ptr<RenderPass> GetRenderPass(const RenderPassDesc& desc);
    std::shared_ptr<BindingSetLayout> GetBindingSetLayout(const std::vector<BindKey>& keys);
    std::shared_ptr<BindingSet> GetBindingSet(const std::shared_ptr<BindingSetLayout>& layout, const std::vector<BindingDesc>& bindings);
    std::shared_ptr<Framebuffer> GetFramebuffer(
        const std::shared_ptr<RenderPass>& render_pass,
        uint32_t width,
        uint32_t height,
        const std::vector<std::shared_ptr<View>>& rtvs,
        const std::shared_ptr<View>& dsv);
    std::shared_ptr<View> GetView(const std::shared_ptr<Program>& program, const BindKey& bind_key, const std::shared_ptr<Resource>& resource, const LazyViewDesc& view_desc);

private:
    Device& m_device;
    std::map<GraphicsPipelineDesc, std::shared_ptr<Pipeline>> m_graphics_object_cache;
    std::map<ComputePipelineDesc, std::shared_ptr<Pipeline>> m_compute_object_cache;
    std::map<RayTracingPipelineDesc, std::shared_ptr<Pipeline>> m_ray_tracing_object_cache;
    std::map<RenderPassDesc, std::shared_ptr<RenderPass>> m_render_pass_cache;
    std::map<std::vector<BindKey>, std::shared_ptr<BindingSetLayout>> m_layout_cache;
    std::map<std::pair<std::shared_ptr<BindingSetLayout>, std::vector<BindingDesc>>, std::shared_ptr<BindingSet>> m_binding_set_cache;
    std::map<std::tuple<std::shared_ptr<RenderPass>, uint32_t, uint32_t, std::vector<std::shared_ptr<View>>, std::shared_ptr<View>>, std::shared_ptr<Framebuffer>> m_framebuffers;
    std::map<std::tuple<std::shared_ptr<Program>, BindKey, std::shared_ptr<Resource>, LazyViewDesc>, std::shared_ptr<View>> m_views;
};
