#pragma once

#include <algorithm>
#include <deque>
#include <array>
#include <set>
#include <unordered_map>
#include <functional>

#include <Instance/BaseTypes.h>

#include <Instance/BaseTypes.h>
#include <Resource/Resource.h>
#include <map>
#include <memory>
#include <vector>

#include <assert.h>
#include "IShaderBlobProvider.h"

class BufferLayout;
class ShaderBase;
class Context;

class ProgramApi
    : public IShaderBlobProvider
{
public:
    ProgramApi(Context& context);

    size_t GetProgramId() const override;
    ShaderBlob GetBlobByType(ShaderType type) const override;
    std::set<ShaderType> GetShaderTypes() const override;

    void SetBindingName(const BindKeyOld& bind_key, const std::string& name);
    const std::string& GetBindingName(const BindKeyOld& bind_key) const;
    void AddAvailableShaderType(ShaderType type);
    void LinkProgram();
    void ApplyBindings();
    void CompileShader(const ShaderBase& shader);
    void SetCBufferLayout(const BindKeyOld& bind_key, BufferLayout& buffer_layout);
    void Attach(const BindKeyOld& bind_key, const ViewDesc& view_desc, const std::shared_ptr<Resource>& res);
    void ClearRenderTarget(uint32_t slot, const std::array<float, 4>& color);
    void ClearDepthStencil(uint32_t ClearFlags, float Depth, uint8_t Stencil);
    void SetRasterizeState(const RasterizerDesc& desc);
    void SetBlendState(const BlendDesc& desc);
    void SetDepthStencilState(const DepthStencilDesc& desc);

protected:
    void OnAttachSRV(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const std::shared_ptr<Resource>& ires);
    void OnAttachUAV(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const std::shared_ptr<Resource>& ires);
    void OnAttachCBV(ShaderType type, uint32_t slot, const std::shared_ptr<Resource>& ires);
    void OnAttachSampler(ShaderType type, const std::string& name, uint32_t slot, const std::shared_ptr<Resource>& ires);
    void OnAttachRTV(uint32_t slot, const ViewDesc& view_desc, const std::shared_ptr<Resource>& ires);
    void OnAttachDSV(const ViewDesc& view_desc, const std::shared_ptr<Resource>& ires);

    std::shared_ptr<View> CreateView(const BindKeyOld& bind_key, const ViewDesc& view_desc, const std::shared_ptr<Resource>& res);
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
    std::vector<std::map<BindKeyOld, std::vector<std::shared_ptr<Resource>>>> m_cbv_buffer;
    std::vector<std::map<BindKeyOld, size_t>> m_cbv_offset;

    std::set<ShaderType> m_shader_types;
private:
    Context& m_context;

    size_t m_program_id;
    std::map<BindKeyOld, std::string> m_binding_names;
};
