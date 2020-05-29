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
#include <cassert>

class BufferLayout;
class ShaderBase;
class Context;

class ProgramApi
{
public:
    ProgramApi(Context& context);

    size_t GetProgramId() const;
    void OnSetViewport(uint32_t width, uint32_t height);
    void ProgramDetach();
    void OnPresent();
    void SetBindingName(const BindKeyOld& bind_key, const std::string& name);
    const std::string& GetBindingName(const BindKeyOld& bind_key) const;
    void AddAvailableShaderType(ShaderType type);
    void LinkProgram();
    void ApplyBindings();
    void CompileShader(const ShaderBase& shader);
    void SetCBufferLayout(const BindKeyOld& bind_key, BufferLayout& buffer_layout);
    void Attach(const BindKeyOld& bind_key, const std::shared_ptr<Resource>& resource, const LazyViewDesc& view_desc);
    void ClearRenderTarget(uint32_t slot, const std::array<float, 4>& color);
    void ClearDepthStencil(uint32_t ClearFlags, float Depth, uint8_t Stencil);
    void SetRasterizeState(const RasterizerDesc& desc);
    void SetBlendState(const BlendDesc& desc);
    void SetDepthStencilState(const DepthStencilDesc& desc);

private:
    void Attach(const BindKeyOld& bind_key, const std::shared_ptr<View>& view);
    void OnAttachSRV(const BindKeyOld& bind_key, const std::shared_ptr<View>& view);
    void OnAttachUAV(const BindKeyOld& bind_key, const std::shared_ptr<View>& view);
    void OnAttachRTV(const BindKeyOld& bind_key, const std::shared_ptr<View>& view);
    void OnAttachDSV(const BindKeyOld& bind_key, const std::shared_ptr<View>& view);

    std::shared_ptr<View> CreateView(const BindKeyOld& bind_key, const std::shared_ptr<Resource>& resource, const LazyViewDesc& view_desc);
    std::shared_ptr<View> FindView(ShaderType shader_type, ViewType view_type, uint32_t slot);

    void SetBinding(const BindKeyOld& bind_key, const std::shared_ptr<View>& view);
    void UpdateCBuffers();

    Context& m_context;
    size_t m_program_id;

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

    std::map<BindKeyOld, std::string> m_binding_names;
    Device& m_device;
    std::vector<std::shared_ptr<Shader>> m_shaders;
    std::map<ShaderType, std::shared_ptr<Shader>> m_shader_by_type;
    std::shared_ptr<Program> m_program;
    std::shared_ptr<Pipeline> m_pipeline;
    std::shared_ptr<BindingSet> m_binding_set;

    std::map<GraphicsPipelineDesc, std::shared_ptr<Pipeline>> m_pso;
    std::map<ComputePipelineDesc, std::shared_ptr<Pipeline>> m_compute_pso;
    std::map<std::tuple<uint32_t, uint32_t, std::vector<std::shared_ptr<View>>, std::shared_ptr<View>>, std::shared_ptr<Framebuffer>> m_framebuffers;
    std::shared_ptr<Framebuffer> m_framebuffer;
    std::map<std::tuple<BindKeyOld, std::shared_ptr<Resource>, LazyViewDesc>, std::shared_ptr<View>> m_views;
    ComputePipelineDesc m_compute_pipeline_desc;
    GraphicsPipelineDesc m_graphic_pipeline_desc;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};
