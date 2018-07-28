#include "CommonProgramApi.h"

static size_t GenId()
{
    static size_t id = 0;
    return ++id;
}

CommonProgramApi::CommonProgramApi(DX12Context& context)
    : m_context(context)
    , m_program_id(GenId())
    , m_view_creater(m_context, *this)
{
}

DescriptorHeapRange CommonProgramApi::GetDescriptor(ShaderType shader_type, const std::string& name, ResourceType res_type, uint32_t slot, const Resource::Ptr& ires)
{
    if (!ires)
        return m_context.GetDescriptorPool().GetEmptyDescriptor(res_type);

    DX12Resource& res = static_cast<DX12Resource&>(*ires);
    auto descriptor = m_context.GetDescriptorPool().GetDescriptor({ m_program_id, shader_type, res_type, slot }, res);
    if (!descriptor.exist)
        m_view_creater.CreateView(shader_type, name, res_type, slot, *ires, descriptor.handle);

    return descriptor.handle;
}

void CommonProgramApi::AttachSRV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& res)
{
    SetBinding(type, ResourceType::kSrv, slot, GetDescriptor(type, name, ResourceType::kSrv, slot, res));
    OnAttachSRV(type, name, slot, res);
}

void CommonProgramApi::AttachUAV(ShaderType type, const std::string & name, uint32_t slot, const Resource::Ptr& res)
{
    SetBinding(type, ResourceType::kUav, slot, GetDescriptor(type, name, ResourceType::kUav, slot, res));
    OnAttachUAV(type, name, slot, res);
}

void CommonProgramApi::AttachCBV(ShaderType type, uint32_t slot, const Resource::Ptr & res)
{
    SetBinding(type, ResourceType::kCbv, slot, GetDescriptor(type, "", ResourceType::kCbv, slot, res));
    OnAttachCBV(type, slot, res);
}

void CommonProgramApi::AttachCBuffer(ShaderType type, const std::string& name, uint32_t slot, BufferLayout& buffer)
{
    m_cbv_layout.emplace(std::piecewise_construct,
        std::forward_as_tuple(type, slot),
        std::forward_as_tuple(buffer));
}

void CommonProgramApi::AttachSampler(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& res)
{
    SetBinding(type, ResourceType::kSampler, slot, GetDescriptor(type, "", ResourceType::kSampler, slot, res));
    OnAttachSampler(type, slot, res);
}

void CommonProgramApi::AttachRTV(uint32_t slot, const Resource::Ptr& res)
{
    SetBinding(ShaderType::kPixel, ResourceType::kRtv, slot, GetDescriptor(ShaderType::kPixel, "", ResourceType::kRtv, slot, res));
    OnAttachRTV(slot, res);
}

void CommonProgramApi::AttachDSV(const Resource::Ptr& res)
{
    SetBinding(ShaderType::kPixel, ResourceType::kDsv, 0, GetDescriptor(ShaderType::kPixel, "", ResourceType::kDsv, 0, res));
    OnAttachDSV(res);
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
