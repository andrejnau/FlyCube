#include "ProgramApi.h"
#include "ProgramApi/BufferLayout.h"
#include <Context/Context.h>

static size_t GenId()
{
    static size_t id = 0;
    return ++id;
}

ProgramApi::ProgramApi(Context& context)
    : m_context(context)
    , m_cbv_data(context.FrameCount)
    , m_program_id(GenId())
    , m_device(*context.m_device)
{
}

size_t ProgramApi::GetProgramId() const
{
    return m_program_id;
}

ShaderBlob ProgramApi::GetBlobByType(ShaderType type) const
{
    return {};
}

std::set<ShaderType> ProgramApi::GetShaderTypes() const
{
    return {};
}

uint32_t ProgramApi::GetStrideByVertexSlot(uint32_t slot)
{
    assert(m_graphic_pipeline_desc.input.at(slot).slot == slot);
    return m_graphic_pipeline_desc.input.at(slot).stride;
}

void ProgramApi::ProgramDetach()
{
    m_framebuffer.reset();
}

void ProgramApi::OnPresent()
{
    m_cbv_data[m_context.GetFrameIndex()].cbv_offset.clear();
}

void ProgramApi::SetBindingName(const BindKeyOld& bind_key, const std::string& name)
{
    m_binding_names[bind_key] = name;
}

const std::string& ProgramApi::GetBindingName(const BindKeyOld& bind_key) const
{
    auto it = m_binding_names.find(bind_key);
    if (it == m_binding_names.end())
    {
        static std::string empty;
        return empty;
    }
    return it->second;
}

void ProgramApi::AddAvailableShaderType(ShaderType type)
{
    m_shader_types.insert(type);
}

void ProgramApi::LinkProgram()
{
    m_program = m_device.CreateProgram(m_shaders);

    if (m_shader_by_type.count(ShaderType::kCompute) || m_shader_by_type.count(ShaderType::kLibrary))
    {
        m_compute_pipeline_desc.program = m_program;
    }
    else if (!m_shader_by_type.count(ShaderType::kLibrary))
    {
        m_graphic_pipeline_desc.program = m_program;
        m_graphic_pipeline_desc.input = m_shader_by_type.at(ShaderType::kVertex)->GetInputLayout();
    }
}

void ProgramApi::ApplyBindings()
{
    UpdateCBuffers();

    if (m_shader_by_type.count(ShaderType::kCompute))
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
    else if (m_shader_by_type.count(ShaderType::kLibrary))
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
    }

    m_binding_set = m_program->CreateBindingSet(descs);
    m_context.m_command_list->BindPipeline(m_pipeline);
    m_context.m_command_list->BindBindingSet(m_binding_set);

    if (m_shader_by_type.count(ShaderType::kCompute) || m_shader_by_type.count(ShaderType::kLibrary))
        return;

    std::vector<std::shared_ptr<View>> rtvs;
    for (auto& render_target : m_graphic_pipeline_desc.rtvs)
    {
        rtvs.emplace_back(FindView(ShaderType::kPixel, ViewType::kRenderTarget, render_target.slot));
    }

    auto prev_framebuffer = m_framebuffer;

    auto dsv = FindView(ShaderType::kPixel, ViewType::kDepthStencil, 0);
    auto key = std::make_pair(rtvs, dsv);
    auto f_it = m_framebuffers.find(key);
    if (f_it == m_framebuffers.end())
    {
        m_framebuffer = m_device.CreateFramebuffer(m_pipeline, rtvs, dsv);
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
        if (m_context.m_is_open_render_pass)
            m_context.m_command_list->EndRenderPass();
    }

    if (!m_context.m_is_open_render_pass)
    {
        m_context.m_command_list->BeginRenderPass(m_framebuffer);
        m_context.m_is_open_render_pass = true;
    }
}

void ProgramApi::CompileShader(const ShaderBase& shader)
{
    ShaderDesc desc = shader;
    if (m_shader_types.count(ShaderType::kGeometry) && shader.type == ShaderType::kVertex)
        desc.define["__INTERNAL_DO_NOT_INVERT_Y__"] = "1";
    m_shaders.emplace_back(m_device.CompileShader(desc));
    m_shader_by_type[shader.type] = m_shaders.back();
}

void ProgramApi::SetCBufferLayout(const BindKeyOld& bind_key, BufferLayout& buffer_layout)
{
    m_cbv_layout.emplace(std::piecewise_construct,
        std::forward_as_tuple(bind_key),
        std::forward_as_tuple(buffer_layout));
}

void ProgramApi::SetBinding(const BindKeyOld& bind_key, const ViewDesc& view_desc, const std::shared_ptr<Resource>& res)
{
    BoundResource bound_res = { res, CreateView(bind_key, view_desc, res) };
    auto it = m_bound_resources.find(bind_key);
    if (it == m_bound_resources.end())
        m_bound_resources.emplace(bind_key, bound_res);
    else
        it->second = bound_res;
}

std::shared_ptr<View> ProgramApi::CreateView(const BindKeyOld& bind_key, const ViewDesc& view_desc, const std::shared_ptr<Resource>& res)
{
    auto it = m_views.find({ bind_key, view_desc, res });
    if (it != m_views.end())
        return it->second;
    decltype(auto) shader = m_shader_by_type.at(bind_key.shader_type);
    ViewDesc desc = view_desc;
    desc.view_type = bind_key.view_type;
    switch (bind_key.view_type)
    {
    case ViewType::kShaderResource:
    case ViewType::kUnorderedAccess:
    {
        ResourceBindingDesc binding_desc = shader->GetResourceBindingDesc(GetBindingName(bind_key));
        desc.dimension = binding_desc.dimension;
        break;
    }
    }
    auto view = m_device.CreateView(res, desc);
    m_views.emplace(std::piecewise_construct, std::forward_as_tuple(bind_key, view_desc, res), std::forward_as_tuple(view));
    return view;
}

std::shared_ptr<View> ProgramApi::FindView(ShaderType shader_type, ViewType view_type, uint32_t slot)
{
    BindKeyOld bind_key = { m_program_id, shader_type, view_type, slot };
    auto it = m_bound_resources.find(bind_key);
    if (it == m_bound_resources.end())
        return {};
    return it->second.view;
}

void ProgramApi::Attach(const BindKeyOld& bind_key, const ViewDesc& view_desc, const std::shared_ptr<Resource>& res)
{
    SetBinding(bind_key, view_desc, res);
    switch (bind_key.view_type)
    {
    case ViewType::kShaderResource:
        OnAttachSRV(bind_key.shader_type, GetBindingName(bind_key), bind_key.slot, view_desc, res);
        break;
    case ViewType::kUnorderedAccess:
        OnAttachUAV(bind_key.shader_type, GetBindingName(bind_key), bind_key.slot, view_desc, res);
        break;
    case ViewType::kConstantBuffer:
        OnAttachCBV(bind_key.shader_type, bind_key.slot, res);
        break;
    case ViewType::kSampler:
        OnAttachSampler(bind_key.shader_type, GetBindingName(bind_key), bind_key.slot, res);
        break;
    case ViewType::kRenderTarget:
        OnAttachRTV(bind_key.slot, view_desc,res);
        break;
    case ViewType::kDepthStencil:
        OnAttachDSV(view_desc, res);
        break;
    }
}

void ProgramApi::ClearRenderTarget(uint32_t slot, const std::array<float, 4>& color)
{
    auto& view = FindView(ShaderType::kPixel, ViewType::kRenderTarget, slot);
    if (!view)
        return;
    if (m_context.m_is_open_render_pass)
    {
        m_context.m_command_list->EndRenderPass();
        m_context.m_is_open_render_pass = false;
    }
    m_context.m_command_list->ResourceBarrier(view->GetResource(), ResourceState::kClearColor);
    m_context.m_command_list->ClearColor(view, color);
    m_context.m_command_list->ResourceBarrier(view->GetResource(), ResourceState::kRenderTarget);
}

void ProgramApi::ClearDepthStencil(uint32_t ClearFlags, float Depth, uint8_t Stencil)
{
    auto& view = FindView(ShaderType::kPixel, ViewType::kDepthStencil, 0);
    if (!view)
        return;
    if (m_context.m_is_open_render_pass)
    {
        m_context.m_command_list->EndRenderPass();
        m_context.m_is_open_render_pass = false;
    }
    m_context.m_command_list->ResourceBarrier(view->GetResource(), ResourceState::kClearDepth);
    m_context.m_command_list->ClearDepth(view, Depth);
    m_context.m_command_list->ResourceBarrier(view->GetResource(), ResourceState::kDepthTarget);
}

void ProgramApi::SetRasterizeState(const RasterizerDesc& desc)
{
    m_graphic_pipeline_desc.rasterizer_desc = desc;
}

void ProgramApi::SetBlendState(const BlendDesc& desc)
{
    m_graphic_pipeline_desc.blend_desc = desc;
}

void ProgramApi::SetDepthStencilState(const DepthStencilDesc& desc)
{
    m_graphic_pipeline_desc.depth_desc = desc;
}

void ProgramApi::UpdateCBuffers()
{
    for (auto& x : m_cbv_layout)
    {
        auto& cbv_buffer = m_cbv_data[m_context.GetFrameIndex()].cbv_buffer;
        auto& cbv_offset = m_cbv_data[m_context.GetFrameIndex()].cbv_offset;
        BufferLayout& buffer_layout = x.second;

        auto& buffer_data = buffer_layout.GetBuffer();
        bool change_buffer = buffer_layout.SyncData();
        change_buffer = change_buffer || !cbv_offset.count(x.first);
        if (change_buffer && cbv_offset.count(x.first))
            ++cbv_offset[x.first];
        if (cbv_offset[x.first] >= cbv_buffer[x.first].size())
            cbv_buffer[x.first].push_back(m_device.CreateBuffer(BindFlag::kConstantBuffer, static_cast<uint32_t>(buffer_data.size()), MemoryType::kUpload));

        auto& res = cbv_buffer[x.first][cbv_offset[x.first]];
        if (change_buffer)
        {
            m_context.UpdateSubresource(res, 0, buffer_layout.GetBuffer().data(), 0, 0);
        }

        Attach(x.first, {}, res);
    }
}

void ProgramApi::OnAttachSRV(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const std::shared_ptr<Resource>& resource)
{
    if (m_context.m_is_open_render_pass)
    {
        m_context.m_command_list->EndRenderPass();
        m_context.m_is_open_render_pass = false;
    }

    if (type == ShaderType::kPixel)
        m_context.m_command_list->ResourceBarrier(resource, ResourceState::kPixelShaderResource);
    else
        m_context.m_command_list->ResourceBarrier(resource, ResourceState::kNonPixelShaderResource);
}

void ProgramApi::OnAttachUAV(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const std::shared_ptr<Resource>& resource)
{
    if (m_context.m_is_open_render_pass)
    {
        m_context.m_command_list->EndRenderPass();
        m_context.m_is_open_render_pass = false;
    }

    m_context.m_command_list->ResourceBarrier(resource, ResourceState::kUnorderedAccess);
}

void ProgramApi::OnAttachCBV(ShaderType type, uint32_t slot, const std::shared_ptr<Resource>& resource)
{
}

void ProgramApi::OnAttachSampler(ShaderType type, const std::string& name, uint32_t slot, const std::shared_ptr<Resource>& resource)
{
}

void ProgramApi::OnAttachRTV(uint32_t slot, const ViewDesc& view_desc, const std::shared_ptr<Resource>& resource)
{
    if (slot >= m_graphic_pipeline_desc.rtvs.size())
        m_graphic_pipeline_desc.rtvs.resize(slot + 1);
    decltype(auto) render_target = m_graphic_pipeline_desc.rtvs[slot];
    render_target.slot = slot;
    render_target.format = resource->GetFormat();
    m_context.m_command_list->ResourceBarrier(resource, ResourceState::kRenderTarget);
}

void ProgramApi::OnAttachDSV(const ViewDesc& view_desc, const std::shared_ptr<Resource>& resource)
{
    m_graphic_pipeline_desc.dsv.format = resource->GetFormat();
    m_context.m_command_list->ResourceBarrier(resource, ResourceState::kDepthTarget);
}
