#pragma once

#include <Context/VKContext.h>
#include "Program/ProgramApi.h"
#include "Program/ShaderBase.h"
#include "Program/BufferLayout.h"

#include <spirv_cross.hpp>
#include <spirv_hlsl.hpp>

#include "VkDescriptorPool.h"
#include "Program/CommonProgramApi.h"
#include "Program/VKViewCreater.h"

class VKProgramApi : public CommonProgramApi
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

    virtual ShaderBlob GetBlobByType(ShaderType type) const override;
    virtual std::set<ShaderType> GetShaderTypes() const override
    {
        std::set<ShaderType> res;
        for (auto & spirv_it : m_spirv)
            res.insert(spirv_it.first);
        return res;
    }

    virtual void OnAttachSRV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires) override;
    virtual void OnAttachUAV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires) override;
    virtual void OnAttachCBV(ShaderType type, uint32_t slot, const Resource::Ptr& ires) override;
    virtual void AttachCBuffer(ShaderType type, const std::string& name, uint32_t slot, BufferLayout& buffer) override;
    virtual void OnAttachSampler(ShaderType type, uint32_t slot, const Resource::Ptr& ires) override;
    virtual void OnAttachRTV(uint32_t slot, const Resource::Ptr& ires) override;
    virtual void OnAttachDSV(const Resource::Ptr& ires) override;


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

    std::map<std::tuple<ShaderType, uint32_t>, Resource::Ptr> m_cbv_buffer;

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

    VKDescriptorManager m_descriptor_pool;
    VKViewCreater m_view_creater;
};
