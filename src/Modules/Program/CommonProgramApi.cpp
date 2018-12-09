#include "CommonProgramApi.h"

static size_t GenId()
{
    static size_t id = 0;
    return ++id;
}

CommonProgramApi::CommonProgramApi()
    : m_program_id(GenId())
{
}

size_t CommonProgramApi::GetProgramId() const
{
    return m_program_id;
}

void CommonProgramApi::AddAvailableShaderType(ShaderType type)
{
    m_shader_types.insert(type);
}

void CommonProgramApi::AttachCBuffer(ShaderType type, const std::string& name, uint32_t slot, BufferLayout& buffer)
{
    m_cbv_layout.emplace(std::piecewise_construct,
        std::forward_as_tuple(type, slot),
        std::forward_as_tuple(buffer));
    m_cbv_name.emplace(std::piecewise_construct,
        std::forward_as_tuple(type, slot),
        std::forward_as_tuple(name));
}

void CommonProgramApi::SetBinding(ShaderType shader_type, ResourceType res_type, uint32_t slot, const std::string& name, const Resource::Ptr& res)
{
    auto it = m_heap_ranges.find({ shader_type, res_type, slot, name });
    if (it == m_heap_ranges.end())
    {
        m_heap_ranges.emplace(std::piecewise_construct,
            std::forward_as_tuple(shader_type, res_type, slot, name),
            std::forward_as_tuple(res));
    }
    else
    {
        it->second = res;
    }
}

void CommonProgramApi::Attach(ShaderType shader_type, ResourceType res_type, uint32_t slot, const std::string& name, const Resource::Ptr& res)
{
    SetBinding(shader_type, res_type, slot, name, res);
    switch (res_type)
    {
    case ResourceType::kSrv:
        OnAttachSRV(shader_type, name, slot, res);
        break;
    case ResourceType::kUav:
        OnAttachUAV(shader_type, name, slot, res);
        break;
    case ResourceType::kCbv:
        OnAttachCBV(shader_type, slot, res);
        break;
    case ResourceType::kSampler:
        OnAttachSampler(shader_type, name, slot, res);
        break;
    case ResourceType::kRtv:
        OnAttachRTV(slot, res);
        break;
    case ResourceType::kDsv:
        OnAttachDSV(res);
        break;
    }
}
