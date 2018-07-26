#pragma once

#include <Context/DX12Context.h>
#include "Program/ProgramApi.h"
#include "Program/ShaderBase.h"
#include <algorithm>
#include <deque>
#include <array>
#include <set>
#include <unordered_map>

#include "Context/DescriptorPool.h"

class CommonProgramApi : public ProgramApi
{
public:
    CommonProgramApi(DX12Context& context);

    virtual void AttachSRV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& res) override;
    virtual void AttachUAV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& res) override;
    virtual void AttachCBV(ShaderType type, uint32_t slot, const Resource::Ptr& res);
    virtual void AttachCBuffer(ShaderType type, const std::string& name, UINT slot, BufferLayout& buffer) override;
    virtual void AttachSampler(ShaderType type, uint32_t slot, const SamplerDesc& desc) override;
    virtual void AttachRTV(uint32_t slot, const Resource::Ptr& ires) override;
    virtual void AttachDSV(const Resource::Ptr& ires) override;

protected:
    virtual DescriptorHeapRange CreateSrv(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires, DescriptorHeapRange& handle) = 0;
    virtual DescriptorHeapRange CreateUAV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires, DescriptorHeapRange& handle) = 0;
    virtual DescriptorHeapRange CreateCBV(ShaderType type, uint32_t slot, const Resource::Ptr& ires, DescriptorHeapRange& handle) = 0;
    virtual DescriptorHeapRange CreateSampler(ShaderType type, uint32_t slot, const SamplerDesc& desc, DescriptorHeapRange& handle) = 0;
    virtual DescriptorHeapRange CreateRTV(uint32_t slot, const Resource::Ptr& ires, DescriptorHeapRange& handle) = 0;
    virtual DescriptorHeapRange CreateDSV(const Resource::Ptr& ires, DescriptorHeapRange& handle) = 0;

    void SetBinding(ShaderType shader_type, ResourceType res_type, uint32_t slot, const DescriptorHeapRange& handle);
    DescriptorHeapRange GetDescriptor(ShaderType shader_type, ResourceType res_type, uint32_t slot, const Resource::Ptr& res);
    

    std::map<std::tuple<ShaderType, ResourceType, uint32_t>, DescriptorHeapRange> m_heap_ranges;
    std::map<std::tuple<ShaderType, uint32_t>, std::reference_wrapper<BufferLayout>> m_cbv_layout;
    std::map<std::tuple<ShaderType, uint32_t>, DescriptorHeapRange> m_sample_cache_range;

    DX12Context& m_context;
    size_t m_program_id;
    bool m_changed_binding = false;
    bool m_changed_om = false;
};
