#pragma once

#include <Context/VKContext.h>
#include "Program/ProgramApi.h"
#include "Program/ShaderBase.h"
#include "Program/BufferLayout.h"

class VKProgramApi : public ProgramApi
{
public:
    VKProgramApi(VKContext& context);

    virtual void SetMaxEvents(size_t) override;
    virtual void LinkProgram() override;
    virtual void UseProgram() override;
    virtual void ApplyBindings() override;
    virtual void CompileShader(const ShaderBase& shader) override;
    virtual void AttachSRV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& res) override;
    virtual void AttachUAV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& res) override;
    virtual void AttachCBuffer(ShaderType type, UINT slot, BufferLayout& buffer_layout) override;
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

    VKContext & m_context;
    std::map<ShaderType, std::vector<uint8_t>> m_spirv;
    std::map<ShaderType, VkShaderModule> m_shaders;
    std::map<ShaderType, std::string> m_shaders_info;

    VkPipeline graphicsPipeline;
};
