#pragma once

#include <Context/DX11Context.h>
#include <Shader/ShaderBase.h>
#include "Program/ProgramApi.h"
#include "Program/BufferLayout.h"
#include "CommonProgramApi.h"
#include "DX11ViewCreater.h"

class DX11ProgramApi : public CommonProgramApi
{
public:
    DX11ProgramApi(DX11Context& context);

    virtual void LinkProgram() override;
    virtual void UseProgram() override;
    virtual void ApplyBindings() override;
    virtual void CompileShader(const ShaderBase& shader) override;

    virtual void ClearRenderTarget(uint32_t slot, const FLOAT ColorRGBA[4]) override;
    virtual void ClearDepthStencil(UINT ClearFlags, FLOAT Depth, UINT8 Stencil) override;
    virtual void SetRasterizeState(const RasterizerDesc& desc) override;
    virtual void SetBlendState(const BlendDesc& desc) override;
    virtual void SetDepthStencilState(const DepthStencilDesc& desc) override;

private:
    void ClearBindings();
    void CreateInputLayout();

    void AttachView(ShaderType type, uint32_t slot, ComPtr<ID3D11ShaderResourceView>& srv);
    void AttachView(ShaderType type, uint32_t slot, ComPtr<ID3D11UnorderedAccessView>& uav);
    void AttachView(ShaderType type, uint32_t slot, ComPtr<ID3D11SamplerState>& sampler);
    void AttachView(ShaderType type, uint32_t slot, ComPtr<ID3D11Resource>& res);

    virtual std::set<ShaderType> GetShaderTypes() const override
    {
        std::set<ShaderType> res;
        for (const auto& x : m_blob_map)
            res.insert(x.first);
        return res;
    }

    virtual ShaderBlob GetBlobByType(ShaderType type) const override
    {
        auto it = m_blob_map.find(type);
        if (it == m_blob_map.end())
            return {};

        return { (uint8_t*)it->second->GetBufferPointer(), it->second->GetBufferSize() };
    }

    virtual void OnAttachSRV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires) override {}
    virtual void OnAttachUAV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires) override {}
    virtual void OnAttachCBV(ShaderType type, uint32_t slot, const Resource::Ptr& ires) override {}
    virtual void OnAttachSampler(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires) override {}
    virtual void OnAttachRTV(uint32_t slot, const Resource::Ptr& ires) override {}
    virtual void OnAttachDSV(const Resource::Ptr& ires) override {}

    virtual View::Ptr CreateView(const BindKey& bind_key, const ViewDesc& view_desc, const Resource::Ptr& res) override
    {
        return m_view_creater.GetView(bind_key, view_desc, GetBindingName(bind_key), res);
    }

    std::map<ShaderType, ComPtr<ID3DBlob>> m_blob_map;
    ComPtr<ID3D11InputLayout> input_layout;
    ComPtr<ID3D11VertexShader> vshader;
    ComPtr<ID3D11PixelShader> pshader;
    ComPtr<ID3D11ComputeShader> cshader;
    ComPtr<ID3D11GeometryShader> gshader;
    DX11Context& m_context;
    DX11ViewCreater m_view_creater;
};
