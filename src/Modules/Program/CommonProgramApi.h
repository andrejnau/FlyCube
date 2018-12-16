#pragma once

#include "Program/ProgramApi.h"
#include "Program/DX12ViewCreater.h"
#include <algorithm>
#include <deque>
#include <array>
#include <set>
#include <unordered_map>
#include <functional>

#include <Shader/ShaderBase.h>
#include <Context/DX12View.h>
#include <Context/BaseTypes.h>

class CommonProgramApi
    : public ProgramApi
    , public IShaderBlobProvider
{
public:
    CommonProgramApi();

    virtual void AddAvailableShaderType(ShaderType type) override;
    virtual void AttachCBuffer(ShaderType type, const std::string& name, uint32_t slot, BufferLayout& buffer) override;
    virtual void Attach(ShaderType shader_type, ResourceType res_type, uint32_t slot, ViewId view_id, const std::string& name, const Resource::Ptr& res) override;
    virtual size_t GetProgramId() const override;

protected:
    virtual void OnAttachSRV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires) = 0;
    virtual void OnAttachUAV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires) = 0;
    virtual void OnAttachCBV(ShaderType type, uint32_t slot, const Resource::Ptr& ires) = 0;
    virtual void OnAttachSampler(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires) = 0;
    virtual void OnAttachRTV(uint32_t slot, const Resource::Ptr& ires) = 0;
    virtual void OnAttachDSV(const Resource::Ptr& ires) = 0;

    void SetBinding(ShaderType shader_type, ResourceType res_type, uint32_t slot, ViewId view_id, const std::string& name, const Resource::Ptr& res);

    std::map<std::tuple<ShaderType, ResourceType, uint32_t, ViewId, std::string>, Resource::Ptr> m_heap_ranges;
    std::map<std::tuple<ShaderType, ResourceType, uint32_t, std::string>, ViewId> m_active_view;
    std::map<std::tuple<ShaderType, uint32_t>, std::reference_wrapper<BufferLayout>> m_cbv_layout;
    std::map<std::tuple<ShaderType, uint32_t>, std::string> m_cbv_name;

    size_t m_program_id;
    std::set<ShaderType> m_shader_types;
};
