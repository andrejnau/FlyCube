#pragma once

#include <Context/DX11Context.h>
#include "Program/ProgramApi.h"
#include "Program/BufferLayout.h"

class DX11ProgramApi : public ProgramApi
{
public:
    DX11ProgramApi(DX11Context& context);

    virtual void SetMaxEvents(size_t) override;
    virtual void UseProgram() override;
    virtual void ApplyBindings() override;
    virtual void OnCompileShader(ShaderType type, const ComPtr<ID3DBlob>& blob) override;
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
    void CreateInputLayout();
    void Attach(ShaderType type, uint32_t slot, ComPtr<ID3D11ShaderResourceView>& srv);
    void Attach(ShaderType type, uint32_t slot, ComPtr<ID3D11UnorderedAccessView>& uav);
    void Attach(ShaderType type, uint32_t slot, ComPtr<ID3D11SamplerState>& sampler);
    void AttachCBV(ShaderType type, uint32_t slot, const Resource::Ptr& ires);
    ComPtr<ID3D11ShaderResourceView> CreateSrv(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires);
    ComPtr<ID3D11UnorderedAccessView> CreateUAV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires);

    ComPtr<ID3D11DepthStencilView> CreateDsv(const Resource::Ptr& ires)
    {
        ComPtr<ID3D11DepthStencilView> dsv;
        if (!ires)
            return dsv;

        auto res = std::static_pointer_cast<DX11Resource>(ires);

        ComPtr<ID3D11Texture2D> tex;
        res->resource.As(&tex);

        D3D11_TEXTURE2D_DESC tex_dec = {};
        tex->GetDesc(&tex_dec);

        if (tex_dec.Format == DXGI_FORMAT_R32_TYPELESS) // TODO
        {
            D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
            dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
            dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
            dsv_desc.Texture2DArray.ArraySize = tex_dec.ArraySize;
            ASSERT_SUCCEEDED(m_context.device->CreateDepthStencilView(tex.Get(), &dsv_desc, &dsv));
        }
        else
        {
            ASSERT_SUCCEEDED(m_context.device->CreateDepthStencilView(tex.Get(), nullptr, &dsv));
        }
        return dsv;
    }

    ComPtr<ID3D11RenderTargetView> CreateRtv(const Resource::Ptr& ires)
    {
        auto res = std::static_pointer_cast<DX11Resource>(ires);

        ComPtr<ID3D11RenderTargetView> rtv;
        if (!ires)
            return rtv;

        ASSERT_SUCCEEDED(m_context.device->CreateRenderTargetView(res->resource.Get(), nullptr, &rtv));
        return rtv;
    }

    std::vector<ComPtr<ID3D11RenderTargetView>> m_rtvs;
    ComPtr<ID3D11DepthStencilView> m_dsv;

    std::map<std::tuple<ShaderType, size_t>, std::reference_wrapper<BufferLayout>> m_cbv_layout;
    std::map<std::tuple<ShaderType, size_t>, Resource::Ptr> m_cbv_buffer;
    std::map<ShaderType, ComPtr<ID3DBlob>> m_blob_map;
    ComPtr<ID3D11InputLayout> input_layout;
    ComPtr<ID3D11VertexShader> vshader;
    ComPtr<ID3D11PixelShader> pshader;
    ComPtr<ID3D11ComputeShader> cshader;
    ComPtr<ID3D11GeometryShader> gshader;
    DX11Context& m_context;
};
