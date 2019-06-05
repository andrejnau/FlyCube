#pragma once

#include <Context/BaseTypes.h>
#include <Resource/Resource.h>
#include <map>
#include <memory>
#include <vector>

#include <assert.h>
#include "IShaderBlobProvider.h"

class BufferLayout;
class ShaderBase;

class ProgramApi : public IShaderBlobProvider
{
public:
    ProgramApi();
    virtual size_t GetProgramId() const override;
    void SetBindingName(const BindKey& bind_key, const std::string& name);
    const std::string& GetBindingName(const BindKey& bind_key) const;
    virtual void AddAvailableShaderType(ShaderType type) {}
    virtual void LinkProgram() = 0;
    virtual void ApplyBindings() = 0;
    virtual void CompileShader(const ShaderBase& shader) = 0;
    virtual void SetCBufferLayout(const BindKey& bind_key, BufferLayout& buffer_layout) = 0;
    virtual void Attach(const BindKey& bind_key, const ViewDesc& view_desc, const Resource::Ptr& res) = 0;
    virtual void ClearRenderTarget(uint32_t slot, const std::array<float, 4>& color) = 0;
    virtual void ClearDepthStencil(uint32_t ClearFlags, float Depth, uint8_t Stencil) = 0;
    virtual void SetRasterizeState(const RasterizerDesc& desc) = 0;
    virtual void SetBlendState(const BlendDesc& desc) = 0;
    virtual void SetDepthStencilState(const DepthStencilDesc& desc) = 0;
protected:
    size_t m_program_id;
    std::map<BindKey, std::string> m_binding_names;
};
