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

void CommonProgramApi::AttachSRV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& res)
{
    Attach(type, name, ResourceType::kSrv, slot, res);
}

void CommonProgramApi::AttachUAV(ShaderType type, const std::string & name, uint32_t slot, const Resource::Ptr& res)
{
    Attach(type, name, ResourceType::kUav, slot, res);
}

void CommonProgramApi::AttachCBV(ShaderType type, uint32_t slot, const std::string& name, const Resource::Ptr & res)
{
    Attach(type, name, ResourceType::kCbv, slot, res);
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

void CommonProgramApi::AttachSampler(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& res)
{
    Attach(type, name, ResourceType::kSampler, slot, res);
}

void CommonProgramApi::AttachRTV(uint32_t slot, const Resource::Ptr& res)
{
    Attach(ShaderType::kPixel, "", ResourceType::kRtv, slot, res);
}

void CommonProgramApi::AttachDSV(const Resource::Ptr& res)
{
    Attach(ShaderType::kPixel, "", ResourceType::kDsv, 0, res);
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

void CommonProgramApi::Attach(ShaderType type, const std::string& name, ResourceType res_type, uint32_t slot, const Resource::Ptr& res)
{
    SetBinding(type, res_type, slot, name, res);
    switch (res_type)
    {
    case ResourceType::kSrv:
        OnAttachSRV(type, name, slot, res);
        break;
    case ResourceType::kUav:
        OnAttachUAV(type, name, slot, res);
        break;
    case ResourceType::kCbv:
        OnAttachCBV(type, slot, res);
        break;
    case ResourceType::kSampler:
        OnAttachSampler(type, slot, res);
        break;
    case ResourceType::kRtv:
        OnAttachRTV(slot, res);
        break;
    case ResourceType::kDsv:
        OnAttachDSV(res);
        break;
    }
}
