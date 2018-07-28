#include "CommonProgramApi.h"

static size_t GenId()
{
    static size_t id = 0;
    return ++id;
}

CommonProgramApi::CommonProgramApi(DX12Context& context)
    : m_context(context)
    , m_program_id(GenId())
{
}

DescriptorHeapRange CommonProgramApi::GetDescriptor(ShaderType shader_type, ResourceType res_type, uint32_t slot, const Resource::Ptr& ires)
{
    if (!ires)
        return m_context.GetDescriptorPool().GetEmptyDescriptor(res_type);

    DX12Resource& res = static_cast<DX12Resource&>(*ires);

    return m_context.GetDescriptorPool().GetDescriptor({ m_program_id, shader_type, res_type, slot }, res);
}

void CommonProgramApi::AttachSRV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& res)
{
    m_changed_binding = true;
    SetBinding(type, ResourceType::kSrv, slot, CreateSrv(type, name, slot, res, GetDescriptor(type, ResourceType::kSrv, slot, res)));
}

void CommonProgramApi::AttachUAV(ShaderType type, const std::string & name, uint32_t slot, const Resource::Ptr& res)
{
    m_changed_binding = true;
    SetBinding(type, ResourceType::kUav, slot, CreateUAV(type, name, slot, res, GetDescriptor(type, ResourceType::kUav, slot, res)));
}

void CommonProgramApi::AttachCBV(ShaderType type, uint32_t slot, const Resource::Ptr & res)
{
    m_changed_binding = true;
    SetBinding(type, ResourceType::kCbv, slot, CreateCBV(type, slot, res, GetDescriptor(type, ResourceType::kCbv, slot, res)));
}

void CommonProgramApi::AttachCBuffer(ShaderType type, const std::string& name, uint32_t slot, BufferLayout& buffer)
{
    m_cbv_layout.emplace(std::piecewise_construct,
        std::forward_as_tuple(type, slot),
        std::forward_as_tuple(buffer));
}

void CommonProgramApi::AttachSampler(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires)
{
    m_changed_binding = true;
    SetBinding(type, ResourceType::kSampler, slot, CreateSampler(type, slot, ires, GetDescriptor(type, ResourceType::kSampler, slot, ires)));
}

void CommonProgramApi::AttachRTV(uint32_t slot, const Resource::Ptr& ires)
{
    m_changed_om = true;
    SetBinding(ShaderType::kPixel, ResourceType::kRtv, slot, CreateRTV(slot, ires, GetDescriptor(ShaderType::kPixel, ResourceType::kRtv, slot, ires)));
}

void CommonProgramApi::AttachDSV(const Resource::Ptr& ires)
{
    m_changed_om = true;
    SetBinding(ShaderType::kPixel, ResourceType::kDsv, 0, CreateDSV(ires, GetDescriptor(ShaderType::kPixel, ResourceType::kDsv, 0, ires)));
}

void CommonProgramApi::SetBinding(ShaderType shader_type, ResourceType res_type, uint32_t slot, const DescriptorHeapRange& handle)
{
    auto it = m_heap_ranges.find({ shader_type, res_type, slot });
    if (it == m_heap_ranges.end())
    {
        m_heap_ranges.emplace(std::piecewise_construct,
            std::forward_as_tuple(shader_type, res_type, slot),
            std::forward_as_tuple(handle));
    }
    else
    {
        it->second = handle;
    }
}
