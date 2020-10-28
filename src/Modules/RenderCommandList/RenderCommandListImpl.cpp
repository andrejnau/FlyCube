#include "RenderCommandList/RenderCommandListImpl.h"
#include <Utilities/FormatHelper.h>

RenderCommandListImpl::RenderCommandListImpl(Device& device, CommandListType type)
    : m_device(device)
{
    m_command_list = m_device.CreateCommandList(type);
}

const std::shared_ptr<CommandList>& RenderCommandListImpl::GetCommandList()
{
    return m_command_list;
}

void RenderCommandListImpl::Reset()
{
    m_lazy_barriers.clear();
    m_resource_state_tracker.clear();

    m_cmd_resources.clear();
    m_bound_resources.clear();
    m_bound_deferred_view.clear();
    m_binding_sets.clear();
    m_resource_lazy_view_descs.clear();
    m_command_list->Reset();
}

void RenderCommandListImpl::Close()
{
    if (!kUseFakeClose)
        m_command_list->Close();
}

void RenderCommandListImpl::CopyBuffer(const std::shared_ptr<Resource>& src_buffer, const std::shared_ptr<Resource>& dst_buffer,
                                       const std::vector<BufferCopyRegion>& regions)
{
    for (const auto& region : regions)
    {
        BufferBarrier(src_buffer, ResourceState::kCopySource);
        BufferBarrier(dst_buffer, ResourceState::kCopyDest);
    }
    m_command_list->CopyBuffer(src_buffer, dst_buffer, regions);
}

void RenderCommandListImpl::CopyBufferToTexture(const std::shared_ptr<Resource>& src_buffer, const std::shared_ptr<Resource>& dst_texture,
                                                const std::vector<BufferToTextureCopyRegion>& regions)
{
    for (const auto& region : regions)
    {
        BufferBarrier(src_buffer, ResourceState::kCopySource);
        ImageBarrier(dst_texture, region.texture_mip_level, 1, region.texture_array_layer, 1, ResourceState::kCopyDest);
    }
    m_command_list->CopyBufferToTexture(src_buffer, dst_texture, regions);
}

void RenderCommandListImpl::CopyTexture(const std::shared_ptr<Resource>& src_texture, const std::shared_ptr<Resource>& dst_texture, const std::vector<TextureCopyRegion>& regions)
{
    for (const auto& region : regions)
    {
        ImageBarrier(src_texture, region.src_mip_level, 1, region.src_array_layer, 1, ResourceState::kCopySource);
        ImageBarrier(dst_texture, region.dst_mip_level, 1, region.dst_array_layer, 1, ResourceState::kCopyDest);
    }
    m_command_list->CopyTexture(src_texture, dst_texture, regions);
}

void RenderCommandListImpl::UpdateSubresource(const std::shared_ptr<Resource>& resource, uint32_t subresource, const void* data, uint32_t row_pitch, uint32_t depth_pitch)
{
    switch (resource->GetMemoryType())
    {
    case MemoryType::kUpload:
        return resource->UpdateUploadBuffer(0, data, resource->GetWidth());
    case MemoryType::kDefault:
        return UpdateSubresourceDefault(resource, subresource, data, row_pitch, depth_pitch);
    }
}

void RenderCommandListImpl::UpdateSubresourceDefault(const std::shared_ptr<Resource>& resource, uint32_t subresource, const void* data, uint32_t row_pitch, uint32_t depth_pitch)
{
    std::shared_ptr<Resource>& upload_resource = m_cmd_resources.emplace_back();

    switch (resource->GetResourceType())
    {
    case ResourceType::kBuffer:
    {
        size_t buffer_size = resource->GetWidth();
        if (!upload_resource)
        {
            upload_resource = m_device.CreateBuffer(BindFlag::kCopySource, buffer_size);
            upload_resource->CommitMemory(MemoryType::kUpload);
        }
        upload_resource->UpdateUploadBuffer(0, data, buffer_size);

        std::vector<BufferCopyRegion> regions;
        auto& region = regions.emplace_back();
        region.num_bytes = buffer_size;
        BufferBarrier(resource, ResourceState::kCopyDest);
        m_command_list->CopyBuffer(upload_resource, resource, regions);
        break;
    }
    case ResourceType::kTexture:
    {
        std::vector<BufferToTextureCopyRegion> regions;
        auto& region = regions.emplace_back();
        region.texture_mip_level = subresource % resource->GetLevelCount();
        region.texture_array_layer = subresource / resource->GetLevelCount();
        region.texture_extent.width = std::max<uint32_t>(1, resource->GetWidth() >> region.texture_mip_level);
        region.texture_extent.height = std::max<uint32_t>(1, resource->GetHeight() >> region.texture_mip_level);
        region.texture_extent.depth = 1;

        size_t num_bytes = 0, row_bytes = 0, num_rows = 0;
        GetFormatInfo(region.texture_extent.width, region.texture_extent.height, resource->GetFormat(), num_bytes, row_bytes, num_rows, m_device.GetTextureDataPitchAlignment());
        region.buffer_row_pitch = row_bytes;

        if (!upload_resource)
        {
            upload_resource = m_device.CreateBuffer(BindFlag::kCopySource, num_bytes);
            upload_resource->CommitMemory(MemoryType::kUpload);
        }
        upload_resource->UpdateUploadBufferWithTextureData(0, row_bytes, num_bytes, data, row_pitch, depth_pitch, num_rows, region.texture_extent.depth);

        ImageBarrier(resource, region.texture_mip_level, 1, region.texture_array_layer, 1, ResourceState::kCopyDest);
        m_command_list->CopyBufferToTexture(upload_resource, resource, regions);
        break;
    }
    }
}

void RenderCommandListImpl::SetViewport(float x, float y, float width, float height)
{
    m_command_list->SetViewport(x, y, width, height);
    m_command_list->SetScissorRect(x, y, width, height);
    m_viewport_width = width;
    m_viewport_height = height;
}

void RenderCommandListImpl::SetScissorRect(int32_t left, int32_t top, uint32_t right, uint32_t bottom)
{
    m_command_list->SetScissorRect(left, top, right, bottom);
}

void RenderCommandListImpl::IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format)
{
    BufferBarrier(resource, ResourceState::kIndexBuffer);
    m_command_list->IASetIndexBuffer(resource, format);
}

void RenderCommandListImpl::IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource)
{
    BufferBarrier(resource, ResourceState::kVertexAndConstantBuffer);
    m_command_list->IASetVertexBuffer(slot, resource);
}

void RenderCommandListImpl::RSSetShadingRateImage(const std::shared_ptr<View>& view)
{
    ViewBarrier(view, ResourceState::kShadingRateSource);
    m_command_list->RSSetShadingRateImage(view);
}

void RenderCommandListImpl::BeginEvent(const std::string& name)
{
    m_command_list->BeginEvent(name);
}

void RenderCommandListImpl::EndEvent()
{
    m_command_list->EndEvent();
}

void RenderCommandListImpl::DrawIndexed(uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location)
{
    ApplyPipeline();
    ApplyBindingSet();
    m_command_list->DrawIndexed(index_count, start_index_location, base_vertex_location);
}

void RenderCommandListImpl::Dispatch(uint32_t thread_group_count_x, uint32_t thread_group_count_y, uint32_t thread_group_count_z)
{
    ApplyPipeline();
    ApplyBindingSet();
    m_command_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z);
}

void RenderCommandListImpl::DispatchMesh(uint32_t thread_group_count_x)
{
    ApplyPipeline();
    ApplyBindingSet();
    m_command_list->DispatchMesh(thread_group_count_x);
}

void RenderCommandListImpl::DispatchRays(uint32_t width, uint32_t height, uint32_t depth)
{
    ApplyPipeline();
    ApplyBindingSet();
    m_command_list->DispatchRays(width, height, depth);
}

void RenderCommandListImpl::BufferBarrier(const std::shared_ptr<Resource>& resource, ResourceState state)
{
    if (!resource)
        return;
    LazyResourceBarrierDesc barrier = {};
    barrier.resource = resource;
    barrier.state = state;
    LazyResourceBarrier({ barrier });
}

void RenderCommandListImpl::ViewBarrier(const std::shared_ptr<View>& view, ResourceState state)
{
    if (!view || !view->GetResource())
        return;
    ImageBarrier(view->GetResource(), view->GetBaseMipLevel(), view->GetLevelCount(), view->GetBaseArrayLayer(), view->GetLayerCount(), state);
}

void RenderCommandListImpl::ImageBarrier(const std::shared_ptr<Resource>& resource, uint32_t base_mip_level, uint32_t level_count, uint32_t base_array_layer, uint32_t layer_count, ResourceState state)
{
    if (!resource)
        return;
    LazyResourceBarrierDesc barrier = {};
    barrier.resource = resource;
    barrier.base_mip_level = base_mip_level;
    barrier.level_count = level_count;
    barrier.base_array_layer = base_array_layer;
    barrier.layer_count = layer_count;
    barrier.state = state;
    LazyResourceBarrier({ barrier });
}

void RenderCommandListImpl::BuildBottomLevelAS(const std::shared_ptr<Resource>& src, const std::shared_ptr<Resource>& dst, const std::vector<RaytracingGeometryDesc>& descs, BuildAccelerationStructureFlags flags)
{
    for (const auto& desc : descs)
    {
        if (desc.vertex.res)
            BufferBarrier(desc.vertex.res, ResourceState::kNonPixelShaderResource);
        if (desc.index.res)
            BufferBarrier(desc.index.res, ResourceState::kNonPixelShaderResource);
    }

    RaytracingASPrebuildInfo prebuild_info = dst->GetRaytracingASPrebuildInfo();
    auto scratch = m_device.CreateBuffer(BindFlag::kRayTracing, src ? prebuild_info.update_scratch_data_size : prebuild_info.build_scratch_data_size);
    scratch->CommitMemory(MemoryType::kDefault);
    m_command_list->BuildBottomLevelAS(src, dst, scratch, 0, descs);
    m_command_list->UAVResourceBarrier(dst);
    m_cmd_resources.emplace_back(scratch);
}

void RenderCommandListImpl::BuildTopLevelAS(const std::shared_ptr<Resource>& src, const std::shared_ptr<Resource>& dst, const std::vector<std::pair<std::shared_ptr<Resource>, glm::mat4>>& geometry, BuildAccelerationStructureFlags flags)
{
    std::vector<RaytracingGeometryInstance> instances;
    for (const auto& mesh : geometry)
    {
        RaytracingGeometryInstance& instance = instances.emplace_back();
        memcpy(&instance.transform, &mesh.second, sizeof(instance.transform));
        instance.instance_id = static_cast<uint32_t>(instances.size() - 1);
        instance.instance_mask = 0xff;
        instance.acceleration_structure_handle = mesh.first->GetAccelerationStructureHandle();
    }

    auto instance_data = m_device.CreateBuffer(BindFlag::kRayTracing, instances.size() * sizeof(instances.back()));
    instance_data->CommitMemory(MemoryType::kUpload);
    instance_data->UpdateUploadBuffer(0, instances.data(), instances.size() * sizeof(instances.back()));

    RaytracingASPrebuildInfo prebuild_info = dst->GetRaytracingASPrebuildInfo();
    auto scratch = m_device.CreateBuffer(BindFlag::kRayTracing, src ? prebuild_info.update_scratch_data_size : prebuild_info.build_scratch_data_size);
    scratch->CommitMemory(MemoryType::kDefault);

    m_command_list->BuildTopLevelAS(src, dst, scratch, 0, instance_data, 0, geometry.size());
    m_command_list->UAVResourceBarrier(dst);

    m_cmd_resources.emplace_back(scratch);
    m_cmd_resources.emplace_back(instance_data);
}

void RenderCommandListImpl::CopyAccelerationStructure(const std::shared_ptr<Resource>& src, const std::shared_ptr<Resource>& dst, CopyAccelerationStructureMode mode)
{
    m_command_list->CopyAccelerationStructure(src, dst, mode);
}

void RenderCommandListImpl::UseProgram(const std::shared_ptr<Program>& program)
{
    m_graphic_pipeline_desc = {};
    m_compute_pipeline_desc = {};
    m_ray_tracing_pipeline_desc = {};

    m_program = program;
    if (m_program->HasShader(ShaderType::kCompute))
    {
        m_compute_pipeline_desc.program = m_program;
    }
    else if (m_program->HasShader(ShaderType::kLibrary))
    {
        m_ray_tracing_pipeline_desc.program = m_program;
    }
    else
    {
        m_graphic_pipeline_desc.program = m_program;
        if (m_program->HasShader(ShaderType::kVertex))
            m_graphic_pipeline_desc.input = m_program->GetShader(ShaderType::kVertex)->GetInputLayout();
    }
    m_bound_resources.clear();
    m_bound_deferred_view.clear();
}

void RenderCommandListImpl::ApplyPipeline()
{
    std::shared_ptr<Pipeline> pipeline;
    if (m_program->HasShader(ShaderType::kCompute))
    {
        auto it = m_compute_pipeline_cache.find(m_compute_pipeline_desc);
        if (it == m_compute_pipeline_cache.end())
        {
            pipeline = m_device.CreateComputePipeline(m_compute_pipeline_desc);
            m_compute_pipeline_cache.emplace(std::piecewise_construct,
                std::forward_as_tuple(m_compute_pipeline_desc),
                std::forward_as_tuple(pipeline));
        }
        else
        {
            pipeline = it->second;
        }
    }
    else if (m_program->HasShader(ShaderType::kLibrary))
    {
        auto it = m_ray_tracing_pipeline_cache.find(m_ray_tracing_pipeline_desc);
        if (it == m_ray_tracing_pipeline_cache.end())
        {
            pipeline = m_device.CreateRayTracingPipeline(m_ray_tracing_pipeline_desc);
            m_ray_tracing_pipeline_cache.emplace(std::piecewise_construct,
                std::forward_as_tuple(m_ray_tracing_pipeline_desc),
                std::forward_as_tuple(pipeline));
        }
        else
        {
            pipeline = it->second;
        }
    }
    else
    {
        auto it = m_graphics_pipeline_cache.find(m_graphic_pipeline_desc);
        if (it == m_graphics_pipeline_cache.end())
        {
            pipeline = m_device.CreateGraphicsPipeline(m_graphic_pipeline_desc);
            m_graphics_pipeline_cache.emplace(std::piecewise_construct,
                std::forward_as_tuple(m_graphic_pipeline_desc),
                std::forward_as_tuple(pipeline));
        }
        else
        {
            pipeline = it->second;
        }
    }

    m_command_list->BindPipeline(pipeline);
}

void RenderCommandListImpl::ApplyBindingSet()
{
    for (auto& x : m_bound_deferred_view)
    {
        auto view = x.second->GetView(*this);
        m_resource_lazy_view_descs.emplace_back(view);
        Attach(x.first, CreateView(x.first, view->resource, view->view_desc));
    }

    std::vector<BindingDesc> descs;
    descs.reserve(m_bound_resources.size());
    for (auto& x : m_bound_resources)
    {
        BindingDesc& desc = descs.emplace_back();
        desc.bind_key = x.first;
        desc.view = x.second;
    }

    auto binding_set = m_program->CreateBindingSet(descs);
    m_command_list->BindBindingSet(binding_set);
    m_binding_sets.emplace_back(binding_set);
}

void RenderCommandListImpl::SetBinding(const BindKey& bind_key, const std::shared_ptr<View>& view)
{
    auto it = m_bound_resources.find(bind_key);
    if (it == m_bound_resources.end())
        m_bound_resources.emplace(bind_key, view);
    else
        it->second = view;
}

std::shared_ptr<View> RenderCommandListImpl::CreateView(const BindKey& bind_key, const std::shared_ptr<Resource>& resource, const LazyViewDesc& view_desc)
{
    auto it = m_views.find({ bind_key, resource, view_desc });
    if (it != m_views.end())
        return it->second;
    decltype(auto) shader = m_program->GetShader(bind_key.shader_type);
    ViewDesc desc = {};
    static_cast<LazyViewDesc&>(desc) = view_desc;
    desc.view_type = bind_key.view_type;
    switch (bind_key.view_type)
    {
    case ViewType::kShaderResource:
    case ViewType::kUnorderedAccess:
    {
        ResourceBindingDesc binding_desc = shader->GetResourceBindingDesc(bind_key);
        desc.dimension = shader->GetResourceBindingDesc(bind_key).dimension;
        desc.stride = shader->GetResourceStride(bind_key);
        break;
    }
    }
    auto view = m_device.CreateView(resource, desc);
    m_views.emplace(std::piecewise_construct, std::forward_as_tuple(bind_key, resource, view_desc), std::forward_as_tuple(view));
    return view;
}

void RenderCommandListImpl::BeginRenderPass(const RenderPassBeginDesc& desc)
{
    RenderPassDesc render_pass_desc = {};
    ClearDesc clear_desc = {};
    render_pass_desc.colors.reserve(desc.colors.size());
    for (const auto& color_desc : desc.colors)
    {
        auto& color = render_pass_desc.colors.emplace_back();
        clear_desc.colors.emplace_back() = color_desc.clear_color;
        if (!color_desc.texture)
            continue;
        color.format = color_desc.texture->GetFormat();
        color.load_op = color_desc.load_op;
        color.store_op = color_desc.store_op;
        render_pass_desc.sample_count = color_desc.texture->GetSampleCount();
    }

    if (desc.depth_stencil.texture)
    {
        render_pass_desc.depth_stencil.format = desc.depth_stencil.texture->GetFormat();
        render_pass_desc.depth_stencil.depth_load_op = desc.depth_stencil.depth_load_op;
        render_pass_desc.depth_stencil.depth_store_op = desc.depth_stencil.depth_store_op;
        render_pass_desc.depth_stencil.stencil_load_op = desc.depth_stencil.stencil_load_op;
        render_pass_desc.depth_stencil.stencil_store_op = desc.depth_stencil.stencil_store_op;
        clear_desc.depth = desc.depth_stencil.clear_depth;
        clear_desc.stencil = desc.depth_stencil.clear_stencil;
    }

    std::shared_ptr<RenderPass> render_pass;
    auto render_pass_it = m_render_pass_cache.find(render_pass_desc);
    if (render_pass_it == m_render_pass_cache.end())
    {
        render_pass = m_device.CreateRenderPass(render_pass_desc);
        m_render_pass_cache.emplace(std::piecewise_construct,
            std::forward_as_tuple(render_pass_desc),
            std::forward_as_tuple(render_pass));
    }
    else
    {
        render_pass = render_pass_it->second;
    }

    m_graphic_pipeline_desc.render_pass = render_pass;

    std::vector<std::shared_ptr<View>> rtvs;
    for (size_t i = 0; i < desc.colors.size(); ++i)
    {
        BindKey bind_key = { ShaderType::kPixel, ViewType::kRenderTarget, i, 0 };
        auto view = CreateView(bind_key, desc.colors[i].texture, desc.colors[i].view_desc);
        ViewBarrier(view, ResourceState::kRenderTarget);
        rtvs.emplace_back(view);
    }

    std::shared_ptr<View> dsv;
    if (desc.depth_stencil.texture)
    {
        BindKey bind_key = { ShaderType::kPixel, ViewType::kDepthStencil, 0, 0 };
        dsv = CreateView(bind_key, desc.depth_stencil.texture, desc.depth_stencil.view_desc);
    }
    ViewBarrier(dsv, ResourceState::kDepthTarget);

    auto key = std::make_tuple(m_viewport_width, m_viewport_height, rtvs, dsv);
    std::shared_ptr<Framebuffer> framebuffer;
    auto framebuffer_it = m_framebuffers.find(key);
    if (framebuffer_it == m_framebuffers.end())
    {
        framebuffer = m_device.CreateFramebuffer(render_pass, m_viewport_width, m_viewport_height, rtvs, dsv);
        m_framebuffers.emplace(std::piecewise_construct,
            std::forward_as_tuple(key),
            std::forward_as_tuple(framebuffer));
    }
    else
    {
        framebuffer = framebuffer_it->second;
    }

    m_command_list->BeginRenderPass(render_pass, framebuffer, clear_desc);
}

void RenderCommandListImpl::EndRenderPass()
{
    m_command_list->EndRenderPass();
    m_graphic_pipeline_desc.render_pass = {};
}

void RenderCommandListImpl::Attach(const BindKey& bind_key, const std::shared_ptr<DeferredView>& view)
{
    m_bound_deferred_view[bind_key] = view;
}

void RenderCommandListImpl::Attach(const BindKey& bind_key, const std::shared_ptr<Resource>& resource, const LazyViewDesc& view_desc)
{
    Attach(bind_key, CreateView(bind_key, resource, view_desc));
}

void RenderCommandListImpl::Attach(const BindKey& bind_key, const std::shared_ptr<View>& view)
{
    SetBinding(bind_key, view);
    switch (bind_key.view_type)
    {
    case ViewType::kShaderResource:
        OnAttachSRV(bind_key, view);
        break;
    case ViewType::kUnorderedAccess:
        OnAttachUAV(bind_key, view);
        break;
    case ViewType::kRenderTarget:
    case ViewType::kDepthStencil:
        assert(false);
        break;
    }
}

void RenderCommandListImpl::SetRasterizeState(const RasterizerDesc& desc)
{
    m_graphic_pipeline_desc.rasterizer_desc = desc;
}

void RenderCommandListImpl::SetBlendState(const BlendDesc& desc)
{
    m_graphic_pipeline_desc.blend_desc = desc;
}

void RenderCommandListImpl::SetDepthStencilState(const DepthStencilDesc& desc)
{
    m_graphic_pipeline_desc.depth_desc = desc;
}

void RenderCommandListImpl::OnAttachSRV(const BindKey& bind_key, const std::shared_ptr<View>& view)
{
    if (bind_key.shader_type == ShaderType::kPixel)
        ViewBarrier(view, ResourceState::kPixelShaderResource);
    else
        ViewBarrier(view, ResourceState::kNonPixelShaderResource);
}

void RenderCommandListImpl::OnAttachUAV(const BindKey& bind_key, const std::shared_ptr<View>& view)
{
    ViewBarrier(view, ResourceState::kUnorderedAccess);
}

ResourceStateTracker& RenderCommandListImpl::GetResourceStateTracker(const std::shared_ptr<Resource>& resource)
{
    auto it = m_resource_state_tracker.find(resource);
    if (it == m_resource_state_tracker.end())
        it = m_resource_state_tracker.emplace(resource, *resource).first;
    return it->second;
}

const std::map<std::shared_ptr<Resource>, ResourceStateTracker>& RenderCommandListImpl::GetResourceStateTrackers() const
{
    return m_resource_state_tracker;
}

const std::vector<ResourceBarrierDesc>& RenderCommandListImpl::GetLazyBarriers() const
{
    return m_lazy_barriers;
}

void RenderCommandListImpl::LazyResourceBarrier(const std::vector<LazyResourceBarrierDesc>& barriers)
{
    std::vector<ResourceBarrierDesc> manual_barriers;
    for (const auto& barrier : barriers)
    {
        auto& state_tracker = GetResourceStateTracker(barrier.resource);
        if (state_tracker.HasResourceState() && barrier.base_mip_level == 0 && barrier.level_count == barrier.resource->GetLevelCount() &&
            barrier.base_array_layer == 0 && barrier.layer_count == barrier.resource->GetLayerCount())
        {
            ResourceBarrierDesc manual_barrier = {};
            manual_barrier.resource = barrier.resource;
            manual_barrier.level_count = barrier.level_count;
            manual_barrier.layer_count = barrier.layer_count;
            manual_barrier.state_before = state_tracker.GetResourceState();
            manual_barrier.state_after = barrier.state;
            state_tracker.SetResourceState(manual_barrier.state_after);
            if (manual_barrier.state_before != ResourceState::kUnknown)
                manual_barriers.emplace_back(manual_barrier);
            else
                m_lazy_barriers.emplace_back(manual_barrier);
        }
        else
        {
            for (uint32_t i = 0; i < barrier.level_count; ++i)
            {
                for (uint32_t j = 0; j < barrier.layer_count; ++j)
                {
                    ResourceBarrierDesc manual_barrier = {};
                    manual_barrier.resource = barrier.resource;
                    manual_barrier.base_mip_level = barrier.base_mip_level + i;
                    manual_barrier.level_count = 1;
                    manual_barrier.base_array_layer = barrier.base_array_layer + j;
                    manual_barrier.layer_count = 1;
                    manual_barrier.state_before = state_tracker.GetSubresourceState(manual_barrier.base_mip_level, manual_barrier.base_array_layer);
                    manual_barrier.state_after = barrier.state;
                    state_tracker.SetSubresourceState(manual_barrier.base_mip_level, manual_barrier.base_array_layer, manual_barrier.state_after);
                    if (manual_barrier.state_before != ResourceState::kUnknown)
                        manual_barriers.emplace_back(manual_barrier);
                    else
                        m_lazy_barriers.emplace_back(manual_barrier);
                }
            }
        }
    }
    if (!manual_barriers.empty())
        m_command_list->ResourceBarrier(manual_barriers);
}

uint64_t& RenderCommandListImpl::GetFenceValue()
{
    return m_fence_value;
}
