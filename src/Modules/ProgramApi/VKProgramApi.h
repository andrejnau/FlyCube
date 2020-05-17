#pragma once

#include <Context/VKContext.h>
#include "Program/ProgramApi.h"
#include "Program/BufferLayout.h"
#include <Shader/ShaderBase.h>

#include <spirv_cross.hpp>
#include <spirv_hlsl.hpp>

#include "Context/VKDescriptorPool.h"
#include "Program/CommonProgramApi.h"
#include "Program/VKViewCreater.h"

class VKProgramApi : public CommonProgramApi
{
public:
    VKProgramApi(VKContext& context);

    vk::ShaderStageFlagBits ShaderType2Bit(ShaderType type);
    vk::ShaderStageFlagBits ExecutionModel2Bit(spv::ExecutionModel model);
    virtual void LinkProgram() override;
    void CreateDRXPipeLine();
    void CreateGrPipeLine();
    void CreateComputePipeLine();
    void CreatePipeLine();
    void UseProgram();
    virtual void ApplyBindings() override;
    virtual View::Ptr CreateView(const BindKey& bind_key, const ViewDesc& view_desc, const Resource::Ptr& res) override;
    virtual void CompileShader(const ShaderBase& shader) override;
    void ParseShader(ShaderType type, const std::vector<uint32_t>& spirv_binary, std::vector<vk::DescriptorSetLayoutBinding>& bindings);
    size_t GetSetNumByShaderType(ShaderType type);
    void ParseShaders();

    void OnPresent();

    vk::RenderPass GetRenderPass() const
    {
        return m_render_pass.get();
    }

    vk::Framebuffer GetFramebuffer() const
    {
        return m_framebuffer.get();
    }

    void RenderPassBegin();

    virtual ShaderBlob GetBlobByType(ShaderType type) const override;
    virtual std::set<ShaderType> GetShaderTypes() const override
    {
        std::set<ShaderType> res;
        for (auto & spirv_it : m_spirv)
            res.insert(spirv_it.first);
        return res;
    }

    virtual void OnAttachSRV(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const Resource::Ptr& ires) override;
    virtual void OnAttachUAV(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const Resource::Ptr& ires) override;
    virtual void OnAttachCBV(ShaderType type, uint32_t slot, const Resource::Ptr& ires) override;
    virtual void OnAttachSampler(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires) override;
    virtual void OnAttachRTV(uint32_t slot, const ViewDesc& view_desc, const Resource::Ptr& ires) override;
    virtual void OnAttachDSV(const ViewDesc& view_desc, const Resource::Ptr& ires) override;


    virtual void ClearRenderTarget(uint32_t slot, const std::array<float, 4>& color) override;
    virtual void ClearDepthStencil(uint32_t ClearFlags, float Depth, uint8_t Stencil) override;
    virtual void SetRasterizeState(const RasterizerDesc& desc) override;
    virtual void SetBlendState(const BlendDesc& desc) override;
    virtual void SetDepthStencilState(const DepthStencilDesc& desc) override;

    vk::UniqueBuffer shaderBindingTable;
    vk::UniqueDeviceMemory shaderBindingTableMemory;

private:

    void CreateInputLayout(
        const std::vector<uint32_t>& spirv_binary,
        std::vector<vk::VertexInputBindingDescription>& binding_desc,
        std::vector<vk::VertexInputAttributeDescription>& attribute_desc);

    void CreateRenderPass(const std::vector<uint32_t>& spirv_binary);

    VKContext& m_context;

    std::map<ShaderType, std::vector<uint32_t>> m_spirv;
    std::map<ShaderType, vk::UniqueShaderModule> m_shaders;
    std::map<ShaderType, const ShaderBase*> m_shaders_info2;
    std::map<ShaderType, size_t> m_shader_type2set;

    vk::UniquePipeline graphicsPipeline;
    vk::UniquePipelineLayout m_pipeline_layout;
    std::vector<vk::UniqueDescriptorSetLayout> m_descriptor_set_layouts;
    std::vector<std::map<vk::DescriptorType, size_t>> m_descriptor_count_by_set;

    struct ShaderRef
    {
        ShaderRef(const std::vector<uint32_t>& spirv_binary)
            : compiler(std::move(spirv_binary))
        {
        }

        spirv_cross::CompilerHLSL compiler;

        struct ResourceRef
        {
            spirv_cross::Resource res;
            vk::DescriptorType descriptor_type;
            uint32_t binding;
        };

        std::map<std::string, ResourceRef> resources;

        spirv_cross::SmallVector<spirv_cross::EntryPoint> entries;
    };

    std::map<ShaderType, ShaderRef> m_shader_ref;

    size_t m_num_rtv = 0;
    
    std::vector<vk::AttachmentDescription> m_attachment_descriptions;
    std::vector<vk::AttachmentReference> m_attachment_references;
    vk::UniqueRenderPass m_render_pass;

    vk::GraphicsPipelineCreateInfo pipelineInfo = {};

    std::vector<vk::VertexInputBindingDescription> binding_desc;
    std::vector<vk::VertexInputAttributeDescription> attribute_desc;
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStageCreateInfo;
    vk::UniqueFramebuffer m_framebuffer;
    std::vector<vk::ImageView> m_attachment_views;
    std::vector<std::pair<vk::Extent2D, size_t>> m_rtv_size;

    VKViewCreater m_view_creater;
    bool m_changed_om = true;
    DepthStencilDesc m_depth_stencil_desc;

    class ClearCache
    {
    public:
        vk::ClearColorValue& GetColor(uint32_t slot)
        {
            if (slot >= color.size())
                color.resize(slot + 1);
            return color[slot];
        }

        vk::ClearDepthStencilValue& GetDepth()
        {
            return depth_stencil;
        }

        vk::AttachmentLoadOp& GetColorLoadOp(uint32_t slot)
        {
            if (slot >= color_load_op.size())
                color_load_op.resize(slot + 1, vk::AttachmentLoadOp::eLoad);
            return color_load_op[slot];
        }

        vk::AttachmentLoadOp& GetDepthLoadOp()
        {
            return depth_load_op;
        }
    private:
        std::vector<vk::ClearColorValue> color;
        vk::ClearDepthStencilValue depth_stencil;
        std::vector<vk::AttachmentLoadOp> color_load_op;
        vk::AttachmentLoadOp depth_load_op = vk::AttachmentLoadOp::eLoad;
    } m_clear_cache;

    size_t msaa_count = 1;
    BlendDesc m_blend_desc;
    RasterizerDesc m_rasterizer_desc;
    bool m_is_compute = false;
    bool m_is_dxr = false;
    std::map<std::map<BindKey, View::Ptr>, std::vector<vk::UniqueDescriptorSet>> m_heap_cache;
};
