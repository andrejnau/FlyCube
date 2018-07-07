#pragma once

#include <Context/VKContext.h>
#include "Program/ProgramApi.h"
#include "Program/ShaderBase.h"
#include "Program/BufferLayout.h"

#include <spirv_cross.hpp>
#include <spirv_hlsl.hpp>

class VKProgramApi : public ProgramApi
{
public:
    VKProgramApi(VKContext& context);

    virtual void SetMaxEvents(size_t) override;
    VkShaderStageFlagBits ShaderType2Bit(ShaderType type);
    virtual void LinkProgram() override;
    void CreateGrPipiLine();
    virtual void UseProgram() override;
    virtual void ApplyBindings() override;
    void RenderPassBegin();
    void RenderPassEnd();
    std::vector<uint8_t> hlsl2spirv(const ShaderBase & shader);
    virtual void CompileShader(const ShaderBase& shader) override;
    void ParseShader(ShaderType type, const std::vector<uint32_t>& spirv_binary, std::vector<VkDescriptorSetLayoutBinding>& bindings);
    size_t GetSetNumByShaderType(ShaderType type);
    void ParseShaders();
    virtual void AttachSRV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& res) override;
    virtual void AttachUAV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& res) override;
    virtual void AttachCBuffer(ShaderType type, const std::string& name, UINT slot, BufferLayout& buffer_layout) override;
    virtual void AttachSampler(ShaderType type, uint32_t slot, const SamplerDesc& desc) override;
    virtual void AttachRTV(uint32_t slot, const Resource::Ptr& ires) override;
    virtual void AttachDSV(const Resource::Ptr& ires) override;
    virtual void ClearRenderTarget(uint32_t slot, const FLOAT ColorRGBA[4]) override;
    virtual void ClearDepthStencil(UINT ClearFlags, FLOAT Depth, UINT8 Stencil) override;
    virtual void SetRasterizeState(const RasterizerDesc& desc) override;
    virtual void SetBlendState(const BlendDesc& desc) override;
    virtual void SetDepthStencilState(const DepthStencilDesc& desc) override;

private:

    void CreateInputLayout(
        const std::vector<uint32_t>& spirv_binary,
        std::vector<VkVertexInputBindingDescription>& binding_desc,
        std::vector<VkVertexInputAttributeDescription>& attribute_desc);

    void CreateRenderPass(const std::vector<uint32_t>& spirv_binary);

    void AttachCBV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires);

    VKContext & m_context;
    std::map<ShaderType, std::vector<uint8_t>> m_spirv;
    std::map<ShaderType, VkShaderModule> m_shaders;
    std::map<ShaderType, std::string> m_shaders_info;
    std::map<ShaderType, const ShaderBase*> m_shaders_info2;
    

    VkPipeline graphicsPipeline;
    VkPipelineLayout m_pipeline_layout;
    PerFrameData<std::vector<VkDescriptorSet>> m_descriptor_sets;

    std::map<ShaderType, size_t> m_shader_type2set;

    struct ShaderRef
    {
        ShaderRef(const std::vector<uint32_t>& spirv_binary)
            : compiler(spirv_cross::CompilerHLSL(std::move(spirv_binary)))
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

    std::map<std::tuple<ShaderType, uint32_t, std::string>, std::reference_wrapper<BufferLayout>> m_cbv_layout;
    std::map<std::tuple<ShaderType, uint32_t, std::string>, Resource::Ptr> m_cbv_buffer;

    size_t m_num_rtv = 0;

    
    std::vector<VkAttachmentDescription> m_color_attachments;
    std::vector<VkAttachmentReference> m_color_attachments_ref;
    VkRenderPass renderPass;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};

    std::vector<VkVertexInputBindingDescription> binding_desc;
    std::vector<VkVertexInputAttributeDescription> attribute_desc;
    std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfo;
    VkFramebuffer m_framebuffer;
    std::vector<VkImageView> m_rtv;
    std::vector<VkExtent2D> m_rtv_size;

    VkSampler m_sampler;
};
