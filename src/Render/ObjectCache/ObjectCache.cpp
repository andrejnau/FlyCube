#include "ObjectCache.h"

ObjectCache::ObjectCache(Device& device)
    : m_device(device)
{
}

std::shared_ptr<Pipeline> ObjectCache::GetPipeline(const GraphicsPipelineDesc& desc)
{
    auto it = m_graphics_object_cache.find(desc);
    if (it == m_graphics_object_cache.end())
    {
        auto pipeline = m_device.CreateGraphicsPipeline(desc);
        it = m_graphics_object_cache.emplace(std::piecewise_construct,
            std::forward_as_tuple(desc),
            std::forward_as_tuple(pipeline)).first;
    }
    return it->second;
}

std::shared_ptr<Pipeline> ObjectCache::GetPipeline(const ComputePipelineDesc& desc)
{
    auto it = m_compute_object_cache.find(desc);
    if (it == m_compute_object_cache.end())
    {
        auto pipeline = m_device.CreateComputePipeline(desc);
        it = m_compute_object_cache.emplace(std::piecewise_construct,
            std::forward_as_tuple(desc),
            std::forward_as_tuple(pipeline)).first;
    }
    return it->second;
}

std::shared_ptr<Pipeline> ObjectCache::GetPipeline(const RayTracingPipelineDesc& desc)
{
    auto it = m_ray_tracing_object_cache.find(desc);
    if (it == m_ray_tracing_object_cache.end())
    {
        auto pipeline = m_device.CreateRayTracingPipeline(desc);
        it = m_ray_tracing_object_cache.emplace(std::piecewise_construct,
            std::forward_as_tuple(desc),
            std::forward_as_tuple(pipeline)).first;
    }
    return it->second;
}

std::shared_ptr<RenderPass> ObjectCache::GetRenderPass(const RenderPassDesc& desc)
{
    auto it = m_render_pass_cache.find(desc);
    if (it == m_render_pass_cache.end())
    {
        auto render_pass = m_device.CreateRenderPass(desc);
        it = m_render_pass_cache.emplace(std::piecewise_construct,
            std::forward_as_tuple(desc),
            std::forward_as_tuple(render_pass)).first;
    }
    return it->second;
}

std::shared_ptr<BindingSetLayout> ObjectCache::GetBindingSetLayout(const std::vector<BindKey>& keys)
{
    auto it = m_layout_cache.find(keys);
    if (it == m_layout_cache.end())
    {
        auto layout = m_device.CreateBindingSetLayout(keys);
        it = m_layout_cache.emplace(std::piecewise_construct,
            std::forward_as_tuple(keys),
            std::forward_as_tuple(layout)).first;
    }
    return it->second;
}

std::shared_ptr<BindingSet> ObjectCache::GetBindingSet(const std::shared_ptr<BindingSetLayout>& layout, const std::vector<BindingDesc>& bindings)
{
    auto it = m_binding_set_cache.find({ layout, bindings });
    if (it == m_binding_set_cache.end())
    {
        auto binding_set = m_device.CreateBindingSet(layout);
        binding_set->WriteBindings(bindings);
        it = m_binding_set_cache.emplace(std::piecewise_construct,
            std::forward_as_tuple(layout, bindings),
            std::forward_as_tuple(binding_set)).first;
    }
    return it->second;
}
