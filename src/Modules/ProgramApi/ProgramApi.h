#pragma once

#include <algorithm>
#include <deque>
#include <array>
#include <set>
#include <functional>

#include <Instance/BaseTypes.h>
#include <Instance/Instance.h>
#include <Shader/ShaderBase.h>
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

    void ProgramDetach();
    void OnPresent();
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

    struct CBufferData
    {
        std::map<BindKeyOld, std::vector<std::shared_ptr<Resource>>> cbv_buffer;
        std::map<BindKeyOld, size_t> cbv_offset;
    };

    std::vector< CBufferData> m_cbv_data;

    std::set<ShaderType> m_shader_types;
private:
    Context& m_context;

    size_t m_program_id;
    std::map<BindKeyOld, std::string> m_binding_names;
    Device& m_device;
    std::vector<std::shared_ptr<Shader>> m_shaders;
    std::map<ShaderType, std::shared_ptr<Shader>> m_shader_by_type;
    std::shared_ptr<Program> m_program;
    std::shared_ptr<Pipeline> m_pipeline;
    std::shared_ptr<BindingSet> m_binding_set;

    template<typename T>
    struct Hasher
    {
        std::size_t operator()(const T& oid) const
        {
            const uint8_t* data = reinterpret_cast<const uint8_t*>(&oid);
            auto size = sizeof(T);
            std::size_t prime = 31;
            std::size_t p_pow = 1;
            std::size_t hash = 0;
            for (size_t i = 0; i < size; ++i)
            {
                hash += (*data + 1ll) * p_pow;
                p_pow *= prime;
                ++data;
            }
            return hash;
        }
    };

    template<typename T>
    class EqualFn
    {
    public:
        bool operator() (const T& t1, const T& t2) const
        {
            return memcmp(&t1, &t2, sizeof(T)) == 0;
        }
    };

    std::map<GraphicsPipelineDesc, std::shared_ptr<Pipeline>> m_pso;
    std::map<ComputePipelineDesc, std::shared_ptr<Pipeline>> m_compute_pso;
    std::map<std::pair<std::vector<std::shared_ptr<View>>, std::shared_ptr<View>>, std::shared_ptr<Framebuffer>> m_framebuffers;
    std::shared_ptr<Framebuffer> m_framebuffer;
    std::map<std::tuple<BindKeyOld, ViewDesc, std::shared_ptr<Resource>>, std::shared_ptr<View>> m_views;
    ComputePipelineDesc m_compute_pipeline_desc;
    GraphicsPipelineDesc m_graphic_pipeline_desc;
};
