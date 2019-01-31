#pragma once

#include <Context/GLContext.h>
#include "Program/ProgramApi.h"
#include <Shader/ShaderBase.h>
#include "Program/BufferLayout.h"

#include <spirv_cross.hpp>
#include <spirv_hlsl.hpp>

#include "Context/VkDescriptorPool.h"
#include "Program/CommonProgramApi.h"
#include "Program/VKViewCreater.h"

class GLProgramApi : public CommonProgramApi
{
public:
    GLProgramApi(GLContext& context);

    virtual void LinkProgram() override;
    virtual void UseProgram() override;
    virtual void ApplyBindings() override;
    virtual void CompileShader(const ShaderBase& shader) override;
    void ParseShader(ShaderType type, const std::vector<uint32_t>& spirv_binary, std::vector<VkDescriptorSetLayoutBinding>& bindings);
    void ParseShaders();
    void ParseShadersOuput();
    void ParseVariable();
    void ParseSSBO();

    void OnPresent();

    virtual ShaderBlob GetBlobByType(ShaderType type) const override;
    virtual std::set<ShaderType> GetShaderTypes() const override
    {
        std::set<ShaderType> res;
        return res;
    }

    virtual void OnAttachSRV(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const Resource::Ptr& ires) override;
    virtual void OnAttachUAV(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const Resource::Ptr& ires) override;
    virtual void OnAttachCBV(ShaderType type, uint32_t slot, const Resource::Ptr& ires) override;
    virtual void OnAttachSampler(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires) override;
    virtual void OnAttachRTV(uint32_t slot, const ViewDesc& view_desc, const Resource::Ptr& ires) override;
    virtual void OnAttachDSV(const ViewDesc& view_desc, const Resource::Ptr& ires) override;

    virtual void ClearRenderTarget(uint32_t slot, const FLOAT ColorRGBA[4]) override;
    virtual void ClearDepthStencil(UINT ClearFlags, FLOAT Depth, UINT8 Stencil) override;
    virtual void SetRasterizeState(const RasterizerDesc& desc) override;
    virtual void SetBlendState(const BlendDesc& desc) override;
    virtual void SetDepthStencilState(const DepthStencilDesc& desc) override;

    virtual View::Ptr CreateView(const BindKey& bind_key, const ViewDesc& view_desc, const Resource::Ptr& res) override
    {
        return {};
    }

    GLuint m_vao;

    virtual void SetCBufferLayout(const BindKey& bind_key, BufferLayout& buffer_layout) override;

private:
    GLContext & m_context;
    std::map<ShaderType, std::string> m_src;
    GLuint m_framebuffer;
    GLuint m_program;
    std::map<std::tuple<ShaderType, uint32_t>, Resource::Ptr> m_cbv_buffer;
    std::map<std::string, GLuint> m_cbv_bindings;
    std::map<std::string, std::pair<GLint, GLint>> m_texture_loc;
    std::map<std::string, Resource::Ptr> m_samplers;
    bool m_is_enabled_blend = false;
};
