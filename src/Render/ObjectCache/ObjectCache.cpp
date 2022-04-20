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

std::shared_ptr<Framebuffer> ObjectCache::GetFramebuffer(const FramebufferDesc& desc)
{
    for (const auto& view : desc.colors)
    {
        if (!view)
        {
            continue;
        }

        decltype(auto) res = view->GetResource();
        if (res->IsBackBuffer())
        {
            return m_device.CreateFramebuffer(desc);
        }
    }

    auto it = m_framebuffers.find(desc);
    if (it == m_framebuffers.end())
    {
        auto framebuffer = m_device.CreateFramebuffer(desc);
        it = m_framebuffers.emplace(std::piecewise_construct,
            std::forward_as_tuple(desc),
            std::forward_as_tuple(framebuffer)).first;
    }
    return it->second;
}

uint32_t GetPlaneSlice(const std::shared_ptr<Resource>& resource, ViewType view_type, ReturnType return_type)
{
    if (!resource)
    {
        return 0;
    }

    switch (view_type)
    {
    case ViewType::kTexture:
        break;
    default:
        return 0;
    }

    gli::format format = resource->GetFormat();
    if (gli::is_depth(format) || gli::is_stencil(format))
    {
        switch (return_type)
        {
        case ReturnType::kUint:
            return 1;
        case ReturnType::kFloat:
            return 0;
        default:
            return 0;
            assert(false);
        }
    }
    return 0;
}

ViewDimension GetViewDimension(const std::shared_ptr<Resource>& resource)
{
    if (resource->GetSampleCount() > 1)
    {
        if (resource->GetLayerCount() > 1)
            return ViewDimension::kTexture2DMSArray;
        else
            return ViewDimension::kTexture2DMS;
    }
    else
    {
        if (resource->GetLayerCount() > 1)
            return ViewDimension::kTexture2DArray;
        else
            return ViewDimension::kTexture2D;
    }
}

std::shared_ptr<View> ObjectCache::GetView(const std::shared_ptr<Program>& program, const BindKey& bind_key, const std::shared_ptr<Resource>& resource, const LazyViewDesc& view_desc)
{
    auto it = m_views.find({ program, bind_key, resource, view_desc });
    if (it != m_views.end())
    {
        return it->second;
    }

    ViewDesc desc = {};
    desc.view_type = bind_key.view_type;
    desc.base_mip_level = view_desc.level;
    desc.level_count = view_desc.count;
    desc.buffer_format = view_desc.buffer_format;

    switch (bind_key.view_type)
    {
    case ViewType::kRenderTarget:
    case ViewType::kDepthStencil:
    case ViewType::kShadingRateSource:
        desc.dimension = GetViewDimension(resource);
        break;
    default:
        decltype(auto) shader = program->GetShader(bind_key.shader_type);
        ResourceBindingDesc binding_desc = shader->GetResourceBinding(bind_key);
        desc.dimension = binding_desc.dimension;
        desc.structure_stride = binding_desc.structure_stride;
        desc.plane_slice = GetPlaneSlice(resource, bind_key.view_type, binding_desc.return_type);
        break;
    }

    auto view = m_device.CreateView(resource, desc);
    if (!resource || !resource->IsBackBuffer())
    {
        m_views.emplace(std::piecewise_construct, std::forward_as_tuple(program, bind_key, resource, view_desc), std::forward_as_tuple(view));
    }
    return view;
}
