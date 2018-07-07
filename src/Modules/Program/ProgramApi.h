#pragma once

#include <Context/BaseTypes.h>
#include <Context/Resource.h>
#include <Utilities/FileUtility.h>
#include <Utilities/DXUtility.h>
#include <map>
#include <memory>
#include <vector>

#include <d3dcompiler.h>
#include <wrl.h>
#include <assert.h>

using namespace Microsoft::WRL;

class BufferLayout;
class ShaderBase;

class ProgramApi
{
public:
    virtual void SetMaxEvents(size_t count) = 0;
    virtual void LinkProgram() = 0;
    virtual void UseProgram() = 0;
    virtual void ApplyBindings() = 0;
    virtual void CompileShader(const ShaderBase& shader) = 0;
    virtual void AttachSRV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires) = 0;
    virtual void AttachUAV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires) = 0;
    virtual void AttachCBuffer(ShaderType type, const std::string& name, UINT slot, BufferLayout& buffer) = 0;
    virtual void AttachSampler(ShaderType type, uint32_t slot, const SamplerDesc& desc) = 0;
    virtual void AttachRTV(uint32_t slot, const Resource::Ptr& ires) = 0;
    virtual void AttachDSV(const Resource::Ptr& ires) = 0;
    virtual void ClearRenderTarget(uint32_t slot, const FLOAT ColorRGBA[4]) = 0;
    virtual void ClearDepthStencil(UINT ClearFlags, FLOAT Depth, UINT8 Stencil) = 0;
    virtual void SetRasterizeState(const RasterizerDesc& desc) = 0;
    virtual void SetBlendState(const BlendDesc& desc) = 0;
    virtual void SetDepthStencilState(const DepthStencilDesc& desc) = 0;
};
