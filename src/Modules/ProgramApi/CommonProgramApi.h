#pragma once

#include "ProgramApi/ProgramApi.h"
#include <algorithm>
#include <deque>
#include <array>
#include <set>
#include <unordered_map>
#include <functional>

#include <Instance/BaseTypes.h>
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
    virtual void SetCBufferLayout(const BindKeyOld& bind_key, BufferLayout& buffer_layout) override;
    virtual void Attach(const BindKeyOld& bind_key, const ViewDesc& view_desc, const std::shared_ptr<Resource>& res) override;

protected:
    virtual void OnAttachSRV(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const std::shared_ptr<Resource>& ires) = 0;
    virtual void OnAttachUAV(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const std::shared_ptr<Resource>& ires) = 0;
    virtual void OnAttachCBV(ShaderType type, uint32_t slot, const std::shared_ptr<Resource>& ires) = 0;
    virtual void OnAttachSampler(ShaderType type, const std::string& name, uint32_t slot, const std::shared_ptr<Resource>& ires) = 0;
    virtual void OnAttachRTV(uint32_t slot, const ViewDesc& view_desc, const std::shared_ptr<Resource>& ires) = 0;
    virtual void OnAttachDSV(const ViewDesc& view_desc, const std::shared_ptr<Resource>& ires) = 0;

    virtual std::shared_ptr<View> CreateView(const BindKeyOld& bind_key, const ViewDesc& view_desc, const std::shared_ptr<Resource>& res) = 0;
    std::shared_ptr<View> FindView(ShaderType shader_type, ResourceType res_type, uint32_t slot);

    void SetBinding(const BindKeyOld& bind_key, const ViewDesc& view_desc, const std::shared_ptr<Resource>& res);
    void UpdateCBuffers();

    struct BoundResource
    {
        std::shared_ptr<Resource> res;
        std::shared_ptr<View> view;
    };
    std::map<BindKeyOld, BoundResource> m_bound_resources;
    std::map<BindKeyOld, std::reference_wrapper<BufferLayout>> m_cbv_layout;
    PerFrameData<std::map<BindKeyOld, std::vector<std::shared_ptr<Resource>>>> m_cbv_buffer;
    PerFrameData<std::map<BindKeyOld, size_t>> m_cbv_offset;

    std::set<ShaderType> m_shader_types;
private:
    Context& m_context;
};
