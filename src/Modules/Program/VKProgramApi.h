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

    VkShaderStageFlagBits ShaderType2Bit(ShaderType type);
    virtual void LinkProgram() override;
    void CreateGrPipeLine();
    void CreateComputePipeLine();
    void CreatePipeLine();
    void UseProgram();
    virtual void ApplyBindings() override;
    virtual View::Ptr CreateView(const BindKey& bind_key, const ViewDesc& view_desc, const Resource::Ptr& res) override;
    virtual void CompileShader(const ShaderBase& shader) override;
    void ParseShader(ShaderType type, const std::vector<uint32_t>& spirv_binary, std::vector<VkDescriptorSetLayoutBinding>& bindings);
    size_t GetSetNumByShaderType(ShaderType type);
    void ParseShaders();

    void OnPresent();

    VkRenderPass GetRenderPass() const
    {
        return m_render_pass;
    }

    VkFramebuffer GetFramebuffer() const
    {
        return m_framebuffer;
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

private:

    void CreateInputLayout(
        const std::vector<uint32_t>& spirv_binary,
        std::vector<VkVertexInputBindingDescription>& binding_desc,
        std::vector<VkVertexInputAttributeDescription>& attribute_desc);

    void CreateRenderPass(const std::vector<uint32_t>& spirv_binary);

    VKContext & m_context;
    std::map<ShaderType, std::vector<uint32_t>> m_spirv;
    std::map<ShaderType, VkShaderModule> m_shaders;
    std::map<ShaderType, std::string> m_shaders_info;
    std::map<ShaderType, const ShaderBase*> m_shaders_info2;
    

    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipeline_layout;
    std::vector<VkDescriptorSetLayout> m_descriptor_set_layouts;
    std::vector<std::map<VkDescriptorType, size_t>> m_descriptor_count_by_set;

    std::vector<VkDescriptorSet> m_descriptor_sets;
    std::map<ShaderType, size_t> m_shader_type2set;

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
            VkDescriptorType descriptor_type;
            uint32_t binding;
        };

        std::map<std::string, ResourceRef> resources;
    };


    std::map<ShaderType, ShaderRef> m_shader_ref;

    size_t m_num_rtv = 0;
    
    std::vector<VkAttachmentDescription> m_attachment_descriptions;
    std::vector<VkAttachmentReference> m_attachment_references;
    VkRenderPass m_render_pass = VK_NULL_HANDLE;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};

    std::vector<VkVertexInputBindingDescription> binding_desc;
    std::vector<VkVertexInputAttributeDescription> attribute_desc;
    std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfo;
    VkFramebuffer m_framebuffer;
    std::vector<VkImageView> m_attachment_views;
    std::vector<std::pair<VkExtent2D, size_t>> m_rtv_size;

    VKViewCreater m_view_creater;
    bool m_changed_om = true;
    DepthStencilDesc m_depth_stencil_desc;

    class ClearCache
    {
    public:
        VkClearColorValue& GetColor(uint32_t slot)
        {
            if (slot >= color.size())
                color.resize(slot + 1);
            return color[slot];
        }

        VkClearDepthStencilValue& GetDepth()
        {
            return depth_stencil;
        }

        VkAttachmentLoadOp& GetColorLoadOp(uint32_t slot)
        {
            if (slot >= color_load_op.size())
                color_load_op.resize(slot + 1, VK_ATTACHMENT_LOAD_OP_LOAD);
            return color_load_op[slot];
        }

        VkAttachmentLoadOp& GetDepthLoadOp()
        {
            return depth_load_op;
        }
    private:
        std::vector<VkClearColorValue> color;
        VkClearDepthStencilValue depth_stencil;
        std::vector<VkAttachmentLoadOp> color_load_op;
        VkAttachmentLoadOp depth_load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
    } m_clear_cache;

    size_t msaa_count = 1;
    BlendDesc m_blend_desc;
    RasterizerDesc m_rasterizer_desc;
    bool m_is_compute = false;
};
