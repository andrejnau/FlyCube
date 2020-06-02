#include "CommandListBox/CommandListBox.h"
#include <Utilities/FormatHelper.h>

CommandListBox::CommandListBox(Device& device)
    : m_device(device)
{
    m_command_list = m_device.CreateCommandList();
}

void CommandListBox::Open()
{
    m_upload.clear();
    m_bound_resources.clear();
    m_bound_deferred_view.clear();
    m_binding_sets.clear();
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

    std::shared_ptr<Resource>& upload_resource = m_upload.emplace_back();

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
        ResourceBarrier(resource, ResourceState::kCopyDest);
        m_command_list->CopyBuffer(upload_resource, resource, regions);
        break;
    }
    case ResourceType::kTexture:
    {
        std::vector<BufferToTextureCopyRegion> regions;
        auto& region = regions.emplace_back();
        region.texture_subresource = subresource;
        region.texture_extent.width = std::max<uint32_t>(1, resource->GetWidth() >> subresource);
        region.texture_extent.height = std::max<uint32_t>(1, resource->GetHeight() >> subresource);
        region.texture_extent.depth = 1;

        size_t num_bytes = 0, row_bytes = 0, num_rows = 0;
        GetFormatInfo(region.texture_extent.width, region.texture_extent.height, resource->GetFormat(), num_bytes, row_bytes, num_rows, m_device.GetTextureDataPitchAlignment());
        region.buffer_row_pitch = row_bytes;

        if (!upload_resource)
            upload_resource = m_device.CreateBuffer(BindFlag::kCopySource, num_bytes, MemoryType::kUpload);
        upload_resource->UpdateSubresource(0, row_bytes, num_bytes, data, row_pitch, depth_pitch, num_rows, region.texture_extent.depth);

        ResourceBarrier(resource, ResourceState::kCopyDest);
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

void CommandListBox::SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
}

void CommandListBox::IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format)
{
    ResourceBarrier(resource, ResourceState::kIndexBuffer);
    m_command_list->IASetIndexBuffer(resource, format);
}

void CommandListBox::IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource)
{
    ResourceBarrier(resource, ResourceState::kVertexAndConstantBuffer);
    m_command_list->IASetVertexBuffer(slot, resource);
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

void CommandListBox::ResourceBarrier(const std::shared_ptr<Resource>& resource, ResourceState state)
{
    if (!resource)
        return;
    ResourceBarrierDesc barrier = {};
    barrier.resource = resource;
    barrier.state_before = resource->GetResourceState();
    barrier.state_after = state;
    barrier.level_count = resource->GetMipLevels();
    barrier.layer_count = resource->GetDepthOrArraySize();
    m_command_list->ResourceBarrier({ barrier });
    resource->SetResourceState(state);
}

std::shared_ptr<Resource> CommandListBox::CreateBottomLevelAS(const BufferDesc& vertex, const BufferDesc& index)
{
    if (m_is_open_render_pass)
    {
        m_command_list->EndRenderPass();
        m_is_open_render_pass = false;
    }
    if (vertex.res)
        ResourceBarrier(vertex.res, ResourceState::kNonPixelShaderResource);
    if (index.res)
        ResourceBarrier(index.res, ResourceState::kNonPixelShaderResource);
    return m_device.CreateBottomLevelAS(m_command_list, vertex, index);
}

std::shared_ptr<Resource> CommandListBox::CreateTopLevelAS(const std::vector<std::pair<std::shared_ptr<Resource>, glm::mat4>>& geometry)
{
    if (m_is_open_render_pass)
    {
        m_command_list->EndRenderPass();
        m_is_open_render_pass = false;
    }
    std::vector<GeometryInstance> instances;
    for (const auto& mesh : geometry)
    {
        GeometryInstance& instance = instances.emplace_back();
        memcpy(&instance.transform, &mesh.second, sizeof(instance.transform));
        instance.instance_id = static_cast<uint32_t>(instances.size() - 1);
        instance.instance_mask = 0xff;
        instance.acceleration_structure_handle = mesh.first->GetAccelerationStructureHandle();
    }

    auto instance_data = m_device.CreateBuffer(BindFlag::kRayTracing, instances.size() * sizeof(instances.back()), MemoryType::kUpload);
    instance_data->UpdateUploadData(instances.data(), 0, instances.size() * sizeof(instances.back()));

    return m_device.CreateTopLevelAS(m_command_list, instance_data, geometry.size());
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
        if (!m_program->HasShader(x.first.shader_type))
            continue;
        Attach(x.first, CreateView(x.first, view.resource, view.view_desc));
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
        desc.view = x.second.view;
        desc.type = x.first.view_type;
        desc.shader = x.first.shader_type;
        desc.name = m_binding_names.at(x.first);
        if (!m_program->HasBinding(desc.shader, desc.type, desc.name))
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
        ResourceBindingDesc binding_desc = shader->GetResourceBindingDesc(m_binding_names.at(bind_key));
        desc.dimension = binding_desc.dimension;
        desc.stride = shader->GetResourceStride(m_binding_names.at(bind_key));
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

void CommandListBox::Attach(const NamedBindKey& bind_key, const std::shared_ptr<DeferredView>& view)
{
    m_bound_deferred_view[bind_key] = view;
    m_binding_names[bind_key] = bind_key.name;
}

void CommandListBox::Attach(const NamedBindKey& bind_key, const std::shared_ptr<Resource>& resource, const LazyViewDesc& view_desc)
{
    m_binding_names[bind_key] = bind_key.name;
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

void CommandListBox::ClearColor(const NamedBindKey& bind_key, const std::array<float, 4>& color)
{
    auto& view = FindView(bind_key.shader_type, bind_key.view_type, bind_key.slot);
    if (!view)
        return;
    if (m_is_open_render_pass)
    {
        m_command_list->EndRenderPass();
        m_is_open_render_pass = false;
    }
    ResourceBarrier(view->GetResource(), ResourceState::kClearColor);
    m_command_list->ClearColor(view, color);
    ResourceBarrier(view->GetResource(), ResourceState::kRenderTarget);
}

void CommandListBox::ClearDepth(const NamedBindKey& bind_key, float depth)
{
    auto& view = FindView(bind_key.shader_type, bind_key.view_type, bind_key.slot);
    if (!view)
        return;
    if (m_is_open_render_pass)
    {
        m_command_list->EndRenderPass();
        m_is_open_render_pass = false;
    }
    ResourceBarrier(view->GetResource(), ResourceState::kClearDepth);
    m_command_list->ClearDepth(view, depth);
    ResourceBarrier(view->GetResource(), ResourceState::kDepthTarget);
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
    if (m_is_open_render_pass)
    {
        m_command_list->EndRenderPass();
        m_is_open_render_pass = false;
    }
    auto resource = view->GetResource();

    if (bind_key.shader_type == ShaderType::kPixel)
        ResourceBarrier(resource, ResourceState::kPixelShaderResource);
    else
        ResourceBarrier(resource, ResourceState::kNonPixelShaderResource);
}

void CommandListBox::OnAttachUAV(const BindKey& bind_key, const std::shared_ptr<View>& view)
{
    if (m_is_open_render_pass)
    {
        m_command_list->EndRenderPass();
        m_is_open_render_pass = false;
    }

    ResourceBarrier(view->GetResource(), ResourceState::kUnorderedAccess);
}

void CommandListBox::OnAttachRTV(const BindKey& bind_key, const std::shared_ptr<View>& view)
{
    if (bind_key.slot >= m_graphic_pipeline_desc.rtvs.size())
        m_graphic_pipeline_desc.rtvs.resize(bind_key.slot + 1);
    decltype(auto) render_target = m_graphic_pipeline_desc.rtvs[bind_key.slot];
    render_target.slot = bind_key.slot;
    auto resource = view->GetResource();
    render_target.format = resource->GetFormat();
    ResourceBarrier(resource, ResourceState::kRenderTarget);
}

void CommandListBox::OnAttachDSV(const BindKey& bind_key, const std::shared_ptr<View>& view)
{
    auto resource = view->GetResource();
    m_graphic_pipeline_desc.dsv.format = resource->GetFormat();
    ResourceBarrier(resource, ResourceState::kDepthTarget);
}
