#include "CommonProgramApi.h"
#include "Program/BufferLayout.h"

void CommonProgramApi::AddAvailableShaderType(ShaderType type)
{
    m_shader_types.insert(type);
}

void CommonProgramApi::SetCBufferLayout(const BindKey& bind_key, BufferLayout& buffer_layout)
{
    m_cbv_layout.emplace(std::piecewise_construct,
        std::forward_as_tuple(bind_key),
        std::forward_as_tuple(buffer_layout));
}

void CommonProgramApi::SetBinding(const BindKey& bind_key, const ViewDesc& view_desc, const Resource::Ptr& res)
{
    BoundResource bound_res = { res, CreateView(bind_key, view_desc, res) };
    auto it = m_bound_resources.find(bind_key);
    if (it == m_bound_resources.end())
        m_bound_resources.emplace(bind_key, bound_res);
    else
        it->second = bound_res;
}

View::Ptr CommonProgramApi::FindView(ShaderType shader_type, ResourceType res_type, uint32_t slot)
{
    BindKey bind_key = { m_program_id, shader_type, res_type, slot };
    auto it = m_bound_resources.find(bind_key);
    if (it == m_bound_resources.end())
        return {};
    return it->second.view;
}

void CommonProgramApi::Attach(const BindKey& bind_key, const ViewDesc& view_desc, const Resource::Ptr& res)
{
    SetBinding(bind_key, view_desc, res);
    switch (bind_key.res_type)
    {
    case ResourceType::kSrv:
        OnAttachSRV(bind_key.shader_type, GetBindingName(bind_key), bind_key.slot, res);
        break;
    case ResourceType::kUav:
        OnAttachUAV(bind_key.shader_type, GetBindingName(bind_key), bind_key.slot, res);
        break;
    case ResourceType::kCbv:
        OnAttachCBV(bind_key.shader_type, bind_key.slot, res);
        break;
    case ResourceType::kSampler:
        OnAttachSampler(bind_key.shader_type, GetBindingName(bind_key), bind_key.slot, res);
        break;
    case ResourceType::kRtv:
        OnAttachRTV(bind_key.slot, res);
        break;
    case ResourceType::kDsv:
        OnAttachDSV(res);
        break;
    }
}

void CommonProgramApi::UpdateCBuffers()
{
    for (auto &x : m_cbv_layout)
    {
        BufferLayout& buffer_layout = x.second;
        auto& buffer_data = buffer_layout.GetBuffer();
        bool change_buffer = buffer_layout.SyncData();
        change_buffer = change_buffer || !m_cbv_offset.get().count(x.first);
        if (change_buffer && m_cbv_offset.get().count(x.first))
            ++m_cbv_offset.get()[x.first];
        if (m_cbv_offset.get()[x.first] >= m_cbv_buffer.get()[x.first].size())
            m_cbv_buffer.get()[x.first].push_back(m_context.CreateBuffer(BindFlag::kCbv, static_cast<uint32_t>(buffer_data.size()), 0));

        auto& res = m_cbv_buffer.get()[x.first][m_cbv_offset.get()[x.first]];
        if (change_buffer)
        {
            m_context.UpdateSubresource(res, 0, buffer_layout.GetBuffer().data(), 0, 0);
        }

        Attach(x.first, {}, res);
    }
}
