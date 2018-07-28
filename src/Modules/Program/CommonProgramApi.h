#pragma once

#include <Context/DX12Context.h>
#include "Program/ProgramApi.h"
#include "Program/ShaderBase.h"
#include <algorithm>
#include <deque>
#include <array>
#include <set>
#include <unordered_map>
#include <functional>

#include "Context/DescriptorPool.h"
#include "DX12ViewCreater.h"

class CommonProgramApi
    : public ProgramApi
    , public IShaderBlobProvider
{
public:
    CommonProgramApi(DX12Context& context);

    virtual void AttachSRV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& res) override;
    virtual void AttachUAV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& res) override;
    virtual void AttachCBV(ShaderType type, uint32_t slot, const Resource::Ptr& res);
    virtual void AttachCBuffer(ShaderType type, const std::string& name, uint32_t slot, BufferLayout& buffer) override;
    virtual void AttachSampler(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires) override;
    virtual void AttachRTV(uint32_t slot, const Resource::Ptr& ires) override;
    virtual void AttachDSV(const Resource::Ptr& ires) override;

protected:
    virtual void OnAttachSRV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires) = 0;
    virtual void OnAttachUAV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires) = 0;
    virtual void OnAttachCBV(ShaderType type, uint32_t slot, const Resource::Ptr& ires) = 0;
    virtual void OnAttachSampler(ShaderType type, uint32_t slot, const Resource::Ptr& ires) = 0;
    virtual void OnAttachRTV(uint32_t slot, const Resource::Ptr& ires) = 0;
    virtual void OnAttachDSV(const Resource::Ptr& ires) = 0;

    void SetBinding(ShaderType shader_type, ResourceType res_type, uint32_t slot, const DescriptorHeapRange& handle);
    DescriptorHeapRange GetDescriptor(ShaderType shader_type, const std::string& name, ResourceType res_type, uint32_t slot, const Resource::Ptr& ires);

    std::map<std::tuple<ShaderType, ResourceType, uint32_t>, DescriptorHeapRange> m_heap_ranges;
    std::map<std::tuple<ShaderType, uint32_t>, std::reference_wrapper<BufferLayout>> m_cbv_layout;

    DX12Context& m_context;
    size_t m_program_id;
    DX12ViewCreater m_view_creater;
};
