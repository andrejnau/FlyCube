#include "CommandListBox/CommandListBox.h"
#include <Utilities/FormatHelper.h>

CommandListBox::CommandListBox(Device& device)
    : m_device(device)
{
    m_command_list = m_device.CreateCommandList();
}

void CommandListBox::Open()
{
    m_cmd_resources.clear();
    m_bound_resources.clear();
    m_bound_deferred_view.clear();
    m_binding_sets.clear();
    m_resource_lazy_view_descs.clear();
    m_command_list->Open();
}

void CommandListBox::Close()
{
    if (m_is_open_render_pass)
    {
        m_command_list->EndRenderPass();
        m_is_open_render_pass = false;
    }
    m_command_list->Close();
}

void CommandListBox::CopyTexture(const std::shared_ptr<Resource>& src_texture, const std::shared_ptr<Resource>& dst_texture, const std::vector<TextureCopyRegion>& regions)
{
    for (const auto& region : regions)
    {
        ImageBarrier(src_texture, region.src_mip_level, 1, region.src_array_layer, 1, ResourceState::kCopySource);
        ImageBarrier(dst_texture, region.dst_mip_level, 1, region.dst_array_layer, 1, ResourceState::kCopyDest);
    }
    m_command_list->CopyTexture(src_texture, dst_texture, regions);
}

void CommandListBox::UpdateSubresource(const std::shared_ptr<Resource>& resource, uint32_t subresource, const void* data, uint32_t row_pitch, uint32_t depth_pitch)
{
    switch (resource->GetMemoryType())
    {
    case MemoryType::kUpload:
        return resource->UpdateUploadData(data, 0, resource->GetWidth());
    case MemoryType::kDefault:
        return UpdateSubresourceDefault(resource, subresource, data, row_pitch, depth_pitch);
    }
}

void CommandListBox::UpdateSubresourceDefault(const std::shared_ptr<Resource>& resource, uint32_t subresource, const void* data, uint32_t row_pitch, uint32_t depth_pitch)
{
    if (m_is_open_render_pass)
    {
        m_command_list->EndRenderPass();
        m_is_open_render_pass = false;
    }

    std::shared_ptr<Resource>& upload_resource = m_cmd_resources.emplace_back();

    switch (resource->GetResourceType())
    {
    case ResourceType::kBuffer:
    {
        size_t buffer_size = resource->GetWidth();
        if (!upload_resource)
            upload_resource = m_device.CreateBuffer(BindFlag::kCopySource, buffer_size, MemoryType::kUpload);
        upload_resource->UpdateUploadData(data, 0, buffer_size);

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
            upload_resource = m_device.CreateBuffer(BindFlag::kCopySource, num_bytes, MemoryType::kUpload);
        upload_resource->UpdateSubresource(0, row_bytes, num_bytes, data, row_pitch, depth_pitch, num_rows, region.texture_extent.depth);

        ImageBarrier(resource, region.texture_mip_level, 1, region.texture_array_layer, 1, ResourceState::kCopyDest);
        m_command_list->CopyBufferToTexture(upload_resource, resource, regions);
        break;
    }
    }
}

void CommandListBox::SetViewport(float width, float height)
{
    m_command_list->SetViewport(width, height);
    m_viewport_width = width;
    m_viewport_height = height;
}

void CommandListBox::SetScissorRect(int32_t left, int32_t top, uint32_t right, uint32_t bottom)
{
    m_command_list->SetScissorRect(left, top, right, bottom);
}

void CommandListBox::IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format)
{
    BufferBarrier(resource, ResourceState::kIndexBuffer);
    m_command_list->IASetIndexBuffer(resource, format);
}

void CommandListBox::IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource)
{
    BufferBarrier(resource, ResourceState::kVertexAndConstantBuffer);
    m_command_list->IASetVertexBuffer(slot, resource);
}

void CommandListBox::RSSetShadingRate(ShadingRate shading_rate, const std::array<ShadingRateCombiner, 2>& combiners)
{
    m_command_list->RSSetShadingRate(shading_rate, combiners);
}

void CommandListBox::RSSetShadingRateImage(const std::shared_ptr<Resource>& resource)
{
    ImageBarrier(resource, 0, 1, 0, 1, ResourceState::kShadingRateSource);
    m_command_list->RSSetShadingRateImage(resource);
}

void CommandListBox::BeginEvent(const std::string& name)
{
    m_command_list->BeginEvent(name);
}

void CommandListBox::EndEvent()
{
    m_command_list->EndEvent();
}

void CommandListBox::DrawIndexed(uint32_t IndexCount, uint32_t StartIndexLocation, int32_t BaseVertexLocation)
{
    ApplyBindings();
    m_command_list->DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
}

void CommandListBox::Dispatch(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ)
{
    ApplyBindings();
    m_command_list->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

void CommandListBox::DispatchRays(uint32_t width, uint32_t height, uint32_t depth)
{
    ApplyBindings();
    m_command_list->DispatchRays(width, height, depth);
}

void CommandListBox::BufferBarrier(const std::shared_ptr<Resource>& resource, ResourceState state)
{
    if (!resource)
        return;
    ResourceBarrierDesc barrier = {};
    barrier.resource = resource;
    barrier.state = state;
    m_command_list->ResourceBarrier({ barrier });
}

void CommandListBox::ViewBarrier(const std::shared_ptr<View>& view, ResourceState state)
{
    if (!view || !view->GetResource())
        return;
    ImageBarrier(view->GetResource(), view->GetBaseMipLevel(), view->GetLevelCount(), view->GetBaseArrayLayer(), view->GetLayerCount(), state);
}

void CommandListBox::ImageBarrier(const std::shared_ptr<Resource>& resource, uint32_t base_mip_level, uint32_t level_count, uint32_t base_array_layer, uint32_t layer_count, ResourceState state)
{
    if (!resource)
        return;
    ResourceBarrierDesc barrier = {};
    barrier.resource = resource;
    barrier.base_mip_level = base_mip_level;
    barrier.level_count = level_count;
    barrier.base_array_layer = base_array_layer;
    barrier.layer_count = layer_count;
    barrier.state = state;
    m_command_list->ResourceBarrier({ barrier });
}

std::shared_ptr<Resource> CommandListBox::CreateBottomLevelAS(const BufferDesc& vertex, const BufferDesc& index)
{
    if (m_is_open_render_pass)
    {
        m_command_list->EndRenderPass();
        m_is_open_render_pass = false;
    }
    if (vertex.res)
        BufferBarrier(vertex.res, ResourceState::kNonPixelShaderResource);
    if (index.res)
        BufferBarrier(index.res, ResourceState::kNonPixelShaderResource);

    auto res = m_device.CreateBottomLevelAS(vertex, index);

    RaytracingASPrebuildInfo prebuild_info = res->GetRaytracingASPrebuildInfo();
    auto scratch = m_device.CreateBuffer(BindFlag::kRayTracing, prebuild_info.build_scratch_data_size, MemoryType::kDefault);

    m_command_list->BuildBottomLevelAS(res, scratch, vertex, index);

    m_cmd_resources.emplace_back(scratch);

    return res;
}

std::shared_ptr<Resource> CommandListBox::CreateTopLevelAS(const std::vector<std::pair<std::shared_ptr<Resource>, glm::mat4>>& geometry)
{
    if (m_is_open_render_pass)
    {
        m_command_list->EndRenderPass();
        m_is_open_render_pass = false;
    }
    std::vector<RaytracingGeometryInstance> instances;
    for (const auto& mesh : geometry)
    {
        RaytracingGeometryInstance& instance = instances.emplace_back();
        memcpy(&instance.transform, &mesh.second, sizeof(instance.transform));
        instance.instance_id = static_cast<uint32_t>(instances.size() - 1);
        instance.instance_mask = 0xff;
        instance.acceleration_structure_handle = mesh.first->GetAccelerationStructureHandle();
    }

    auto instance_data = m_device.CreateBuffer(BindFlag::kRayTracing, instances.size() * sizeof(instances.back()), MemoryType::kUpload);
    instance_data->UpdateUploadData(instances.data(), 0, instances.size() * sizeof(instances.back()));

    auto res = m_device.CreateTopLevelAS(geometry.size());
    RaytracingASPrebuildInfo prebuild_info = res->GetRaytracingASPrebuildInfo();
    auto scratch = m_device.CreateBuffer(BindFlag::kRayTracing, prebuild_info.build_scratch_data_size, MemoryType::kDefault);

    m_command_list->BuildTopLevelAS(res, scratch, instance_data, geometry.size());

    m_cmd_resources.emplace_back(scratch);
    m_cmd_resources.emplace_back(instance_data);

    return res;
}

void CommandListBox::ReleaseRequest(const std::shared_ptr<Resource>& resource)
{
    m_cmd_resources.emplace_back(resource);
}

void CommandListBox::UseProgram(std::shared_ptr<Program>& program)
{
    if (m_program != program)
    {
        if (m_is_open_render_pass)
        {
            m_command_list->EndRenderPass();
            m_is_open_render_pass = false;
        }
    }

    m_graphic_pipeline_desc = {};
    m_compute_pipeline_desc = {};

    m_framebuffer.reset();
    m_program = program;
    if (m_program->HasShader(ShaderType::kCompute) || m_program->HasShader(ShaderType::kLibrary))
    {
        m_compute_pipeline_desc.program = m_program;
    }
    else
    {
        m_graphic_pipeline_desc.program = m_program;
        m_graphic_pipeline_desc.input = m_program->GetShader(ShaderType::kVertex)->GetInputLayout();
    }
    m_bound_resources.clear();
}

void CommandListBox::ApplyBindings()
{
    if (m_program->HasShader(ShaderType::kCompute))
    {
        auto it = m_compute_pso.find(m_compute_pipeline_desc);
        if (it == m_compute_pso.end())
        {
            m_pipeline = m_device.CreateComputePipeline(m_compute_pipeline_desc);
            m_compute_pso.emplace(std::piecewise_construct,
                std::forward_as_tuple(m_compute_pipeline_desc),
                std::forward_as_tuple(m_pipeline));
        }
        else
        {
            m_pipeline = it->second;
        }
    }
    else if (m_program->HasShader(ShaderType::kLibrary))
    {
        auto it = m_compute_pso.find(m_compute_pipeline_desc);
        if (it == m_compute_pso.end())
        {
            m_pipeline = m_device.CreateRayTracingPipeline(m_compute_pipeline_desc);
            m_compute_pso.emplace(std::piecewise_construct,
                std::forward_as_tuple(m_compute_pipeline_desc),
                std::forward_as_tuple(m_pipeline));
        }
        else
        {
            m_pipeline = it->second;
        }
    }
    else
    {
        auto it = m_pso.find(m_graphic_pipeline_desc);
        if (it == m_pso.end())
        {
            m_pipeline = m_device.CreateGraphicsPipeline(m_graphic_pipeline_desc);
            m_pso.emplace(std::piecewise_construct,
                std::forward_as_tuple(m_graphic_pipeline_desc),
                std::forward_as_tuple(m_pipeline));
        }
        else
        {
            m_pipeline = it->second;
        }
    }

    for (auto& x : m_bound_deferred_view)
    {
        auto view = x.second->GetView(*this);
        m_resource_lazy_view_descs.emplace_back(view);
        if (!m_program->HasShader(x.first.shader_type))
            continue;
        Attach(x.first, CreateView(x.first, view->resource, view->view_desc));
    }

    std::vector<BindingDesc> descs;
    for (auto& x : m_bound_resources)
    {
        switch (x.first.view_type)
        {
        case ViewType::kRenderTarget:
        case ViewType::kDepthStencil:
            continue;
        }
        decltype(auto) desc = descs.emplace_back();
        desc.bind_key = x.first;
        desc.view = x.second.view;
        if (!m_program->HasBinding(desc.bind_key))
            descs.pop_back();
    }

    m_binding_set = m_program->CreateBindingSet(descs);
    m_binding_sets.emplace_back(m_binding_set);
    m_command_list->BindPipeline(m_pipeline);
    m_command_list->BindBindingSet(m_binding_set);

    if (m_program->HasShader(ShaderType::kCompute) || m_program->HasShader(ShaderType::kLibrary))
        return;

    std::vector<std::shared_ptr<View>> rtvs;
    for (auto& render_target : m_graphic_pipeline_desc.rtvs)
    {
        rtvs.emplace_back(FindView(ShaderType::kPixel, ViewType::kRenderTarget, render_target.slot));
    }

    auto prev_framebuffer = m_framebuffer;

    auto dsv = FindView(ShaderType::kPixel, ViewType::kDepthStencil, 0);
    auto key = std::make_tuple(m_viewport_width, m_viewport_height, rtvs, dsv);
    auto f_it = m_framebuffers.find(key);
    if (f_it == m_framebuffers.end())
    {
        m_framebuffer = m_device.CreateFramebuffer(m_pipeline, m_viewport_width, m_viewport_height, rtvs, dsv);
        m_framebuffers.emplace(std::piecewise_construct,
            std::forward_as_tuple(key),
            std::forward_as_tuple(m_framebuffer));
    }
    else
    {
        m_framebuffer = f_it->second;
    }

    if (prev_framebuffer != m_framebuffer)
    {
        if (m_is_open_render_pass)
            m_command_list->EndRenderPass();
    }

    if (!m_is_open_render_pass)
    {
        m_command_list->BeginRenderPass(m_framebuffer);
        m_is_open_render_pass = true;
    }
}

void CommandListBox::SetBinding(const BindKey& bind_key, const std::shared_ptr<View>& view)
{
    BoundResource bound_res = { view->GetResource(), view };
    auto it = m_bound_resources.find(bind_key);
    if (it == m_bound_resources.end())
        m_bound_resources.emplace(bind_key, bound_res);
    else
        it->second = bound_res;
}

std::shared_ptr<View> CommandListBox::CreateView(const BindKey& bind_key, const std::shared_ptr<Resource>& resource, const LazyViewDesc& view_desc)
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

std::shared_ptr<View> CommandListBox::FindView(ShaderType shader_type, ViewType view_type, uint32_t slot)
{
    BindKey bind_key = { shader_type, view_type, slot };
    auto it = m_bound_resources.find(bind_key);
    if (it == m_bound_resources.end())
        return {};
    return it->second.view;
}

void CommandListBox::Attach(const BindKey& bind_key, const std::shared_ptr<DeferredView>& view)
{
    m_bound_deferred_view[bind_key] = view;
}

void CommandListBox::Attach(const BindKey& bind_key, const std::shared_ptr<Resource>& resource, const LazyViewDesc& view_desc)
{
    Attach(bind_key, CreateView(bind_key, resource, view_desc));
}

void CommandListBox::Attach(const BindKey& bind_key, const std::shared_ptr<View>& view)
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
        OnAttachRTV(bind_key, view);
        break;
    case ViewType::kDepthStencil:
        OnAttachDSV(bind_key, view);
        break;
    }
}

void CommandListBox::ClearColor(const BindKey& bind_key, const std::array<float, 4>& color)
{
    auto& view = FindView(bind_key.shader_type, bind_key.view_type, bind_key.slot);
    if (!view)
        return;
    if (m_is_open_render_pass)
    {
        m_command_list->EndRenderPass();
        m_is_open_render_pass = false;
    }
    ViewBarrier(view, ResourceState::kClearColor);
    m_command_list->ClearColor(view, color);
    ViewBarrier(view, ResourceState::kRenderTarget);
}

void CommandListBox::ClearDepth(const BindKey& bind_key, float depth)
{
    auto& view = FindView(bind_key.shader_type, bind_key.view_type, bind_key.slot);
    if (!view)
        return;
    if (m_is_open_render_pass)
    {
        m_command_list->EndRenderPass();
        m_is_open_render_pass = false;
    }
    ViewBarrier(view, ResourceState::kClearDepth);
    m_command_list->ClearDepth(view, depth);
    ViewBarrier(view, ResourceState::kDepthTarget);
}

void CommandListBox::SetRasterizeState(const RasterizerDesc& desc)
{
    m_graphic_pipeline_desc.rasterizer_desc = desc;
}

void CommandListBox::SetBlendState(const BlendDesc& desc)
{
    m_graphic_pipeline_desc.blend_desc = desc;
}

void CommandListBox::SetDepthStencilState(const DepthStencilDesc& desc)
{
    m_graphic_pipeline_desc.depth_desc = desc;
}

void CommandListBox::OnAttachSRV(const BindKey& bind_key, const std::shared_ptr<View>& view)
{
    if (bind_key.shader_type == ShaderType::kPixel)
        ViewBarrier(view, ResourceState::kPixelShaderResource);
    else
        ViewBarrier(view, ResourceState::kNonPixelShaderResource);
}

void CommandListBox::OnAttachUAV(const BindKey& bind_key, const std::shared_ptr<View>& view)
{
    ViewBarrier(view, ResourceState::kUnorderedAccess);
}

void CommandListBox::OnAttachRTV(const BindKey& bind_key, const std::shared_ptr<View>& view)
{
    if (bind_key.slot >= m_graphic_pipeline_desc.rtvs.size())
        m_graphic_pipeline_desc.rtvs.resize(bind_key.slot + 1);
    decltype(auto) render_target = m_graphic_pipeline_desc.rtvs[bind_key.slot];
    render_target.slot = bind_key.slot;
    auto resource = view->GetResource();
    render_target.format = resource->GetFormat();
    render_target.sample_count = resource->GetSampleCount();
    ViewBarrier(view, ResourceState::kRenderTarget);
}

void CommandListBox::OnAttachDSV(const BindKey& bind_key, const std::shared_ptr<View>& view)
{
    auto resource = view->GetResource();
    m_graphic_pipeline_desc.dsv.format = resource->GetFormat();
    m_graphic_pipeline_desc.dsv.sample_count = resource->GetSampleCount();
    ViewBarrier(view, ResourceState::kDepthTarget);
}
