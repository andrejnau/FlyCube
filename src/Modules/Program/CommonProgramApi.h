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
#include <Context/Context.h>

class CommonProgramApi
    : public ProgramApi
{
public:
    CommonProgramApi(Context& context)
        : m_context(context)
        , m_cbv_buffer(context)
        , m_cbv_offset(context)
    {
    }

    virtual void AddAvailableShaderType(ShaderType type) override;
    virtual void SetCBufferLayout(const BindKey& bind_key, BufferLayout& buffer_layout) override;
    virtual void Attach(const BindKey& bind_key, const ViewDesc& view_desc, const Resource::Ptr& res) override;

protected:
    virtual void OnAttachSRV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires) = 0;
    virtual void OnAttachUAV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires) = 0;
    virtual void OnAttachCBV(ShaderType type, uint32_t slot, const Resource::Ptr& ires) = 0;
    virtual void OnAttachSampler(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires) = 0;
    virtual void OnAttachRTV(uint32_t slot, const Resource::Ptr& ires) = 0;
    virtual void OnAttachDSV(const Resource::Ptr& ires) = 0;

    virtual View::Ptr CreateView(const BindKey& bind_key, const ViewDesc& view_desc, const Resource::Ptr& res) = 0;
    View::Ptr FindView(ShaderType shader_type, ResourceType res_type, uint32_t slot);

    void SetBinding(const BindKey& bind_key, const ViewDesc& view_desc, const Resource::Ptr& res);
    void UpdateCBuffers();

    struct BoundResource
    {
        Resource::Ptr res;
        View::Ptr view;
    };
    std::map<BindKey, BoundResource> m_bound_resources;
    std::map<BindKey, std::reference_wrapper<BufferLayout>> m_cbv_layout;
    PerFrameData<std::map<BindKey, std::vector<Resource::Ptr>>> m_cbv_buffer;
    PerFrameData<std::map<BindKey, size_t>> m_cbv_offset;

    std::set<ShaderType> m_shader_types;
private:
    Context& m_context;
};

