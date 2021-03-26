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

private:
    Device& m_device;
    std::map<GraphicsPipelineDesc, std::shared_ptr<Pipeline>> m_graphics_object_cache;
    std::map<ComputePipelineDesc, std::shared_ptr<Pipeline>> m_compute_object_cache;
    std::map<RayTracingPipelineDesc, std::shared_ptr<Pipeline>> m_ray_tracing_object_cache;
    std::map<RenderPassDesc, std::shared_ptr<RenderPass>> m_render_pass_cache;
    std::map<std::vector<BindKey>, std::shared_ptr<BindingSetLayout>> m_layout_cache;
    std::map<std::pair<std::shared_ptr<BindingSetLayout>, std::vector<BindingDesc>>, std::shared_ptr<BindingSet>> m_binding_set_cache;
};
