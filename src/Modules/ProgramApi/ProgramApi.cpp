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
    , m_cbv_buffer(context.FrameCount)
    , m_cbv_offset(context.FrameCount)
    , m_program_id(GenId())
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
}

void ProgramApi::ApplyBindings()
{
}

void ProgramApi::CompileShader(const ShaderBase& shader)
{
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
    return {};
}

std::shared_ptr<View> ProgramApi::FindView(ShaderType shader_type, ResourceType res_type, uint32_t slot)
{
    BindKeyOld bind_key = { m_program_id, shader_type, res_type, slot };
    auto it = m_bound_resources.find(bind_key);
    if (it == m_bound_resources.end())
        return {};
    return it->second.view;
}

void ProgramApi::Attach(const BindKeyOld& bind_key, const ViewDesc& view_desc, const std::shared_ptr<Resource>& res)
{
    SetBinding(bind_key, view_desc, res);
    switch (bind_key.res_type)
    {
    case ResourceType::kSrv:
        OnAttachSRV(bind_key.shader_type, GetBindingName(bind_key), bind_key.slot, view_desc, res);
        break;
    case ResourceType::kUav:
        OnAttachUAV(bind_key.shader_type, GetBindingName(bind_key), bind_key.slot, view_desc, res);
        break;
    case ResourceType::kCbv:
        OnAttachCBV(bind_key.shader_type, bind_key.slot, res);
        break;
    case ResourceType::kSampler:
        OnAttachSampler(bind_key.shader_type, GetBindingName(bind_key), bind_key.slot, res);
        break;
    case ResourceType::kRtv:
        OnAttachRTV(bind_key.slot, view_desc,res);
        break;
    case ResourceType::kDsv:
        OnAttachDSV(view_desc, res);
        break;
    }
}

void ProgramApi::ClearRenderTarget(uint32_t slot, const std::array<float, 4>& color)
{
}

void ProgramApi::ClearDepthStencil(uint32_t ClearFlags, float Depth, uint8_t Stencil)
{
}

void ProgramApi::SetRasterizeState(const RasterizerDesc& desc)
{
}

void ProgramApi::SetBlendState(const BlendDesc& desc)
{
}

void ProgramApi::SetDepthStencilState(const DepthStencilDesc& desc)
{
}

void ProgramApi::UpdateCBuffers()
{
    for (auto &x : m_cbv_layout)
    {
        BufferLayout& buffer_layout = x.second;
        auto& buffer_data = buffer_layout.GetBuffer();
        bool change_buffer = buffer_layout.SyncData();
        change_buffer = change_buffer || !m_cbv_offset[m_context.m_frame_index].count(x.first);
        if (change_buffer && m_cbv_offset[m_context.m_frame_index].count(x.first))
            ++m_cbv_offset[m_context.m_frame_index][x.first];
        if (m_cbv_offset[m_context.m_frame_index][x.first] >= m_cbv_buffer[m_context.m_frame_index][x.first].size())
            m_cbv_buffer[m_context.m_frame_index][x.first].push_back(m_context.CreateBuffer(BindFlag::kCbv, static_cast<uint32_t>(buffer_data.size()), 0));

        auto& res = m_cbv_buffer[m_context.m_frame_index][x.first][m_cbv_offset[m_context.m_frame_index][x.first]];
        if (change_buffer)
        {
            m_context.UpdateSubresource(res, 0, buffer_layout.GetBuffer().data(), 0, 0);
        }

        Attach(x.first, {}, res);
    }
}

void ProgramApi::OnAttachSRV(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const std::shared_ptr<Resource>& ires)
{
}

void ProgramApi::OnAttachUAV(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const std::shared_ptr<Resource>& ires)
{
}

void ProgramApi::OnAttachCBV(ShaderType type, uint32_t slot, const std::shared_ptr<Resource>& ires)
{
}

void ProgramApi::OnAttachSampler(ShaderType type, const std::string& name, uint32_t slot, const std::shared_ptr<Resource>& ires)
{
}

void ProgramApi::OnAttachRTV(uint32_t slot, const ViewDesc& view_desc, const std::shared_ptr<Resource>& ires)
{
}

void ProgramApi::OnAttachDSV(const ViewDesc& view_desc, const std::shared_ptr<Resource>& ires)
{
}
