#pragma once

#include "Context/Context.h"
#include <d3d11.h>
#include <d3d11_1.h>
#include <DXGI1_4.h>
#include <wrl.h>
#include <GLFW/glfw3.h>
#include <vector>

using namespace Microsoft::WRL;

class DX11Context : public Context
{
public:
    DX11Context(GLFWwindow* window, int width, int height);
    virtual ComPtr<IUnknown> GetBackBuffer() override;
    virtual void Present() override;
    virtual void DrawIndexed(UINT IndexCount) override;
    virtual void Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ) override;
    virtual void SetViewport(int width, int height) override;
    virtual void OMSetRenderTargets(std::vector<ComPtr<IUnknown>> rtv, ComPtr<IUnknown> dsv) override;
    virtual void ClearRenderTarget(ComPtr<IUnknown> rtv, const FLOAT ColorRGBA[4]) override;
    virtual void ClearDepthStencil(ComPtr<IUnknown> dsv, UINT ClearFlags, FLOAT Depth, UINT8 Stencil) override;
    virtual ComPtr<IUnknown> CreateTexture(uint32_t bind_flag, DXGI_FORMAT format, uint32_t msaa_count, int width, int height, int depth) override;
    virtual ComPtr<IUnknown> CreateSamplerAnisotropic() override;
    virtual ComPtr<IUnknown> CreateSamplerShadow() override;
    virtual ComPtr<IUnknown> CreateShadowRSState() override;
    virtual void RSSetState(ComPtr<IUnknown> state) override;
    virtual std::unique_ptr<ProgramApi> CreateProgram() override;
    virtual ComPtr<IUnknown> CreateBuffer(uint32_t bind_flag, UINT buffer_size, size_t stride, const std::string& name) override;

    virtual void UpdateSubresource(ComPtr<IUnknown> ires, UINT DstSubresource, const void *pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch) override;

    virtual void BeginEvent(LPCWSTR Name) override;
    virtual void EndEvent() override;

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> device_context;
    ComPtr<ID3DUserDefinedAnnotation> perf;
private:
    ComPtr<ID3D11DepthStencilView> ToDsv(const ComPtr<IUnknown>& ires)
    {
        ComPtr<ID3D11DepthStencilView> dsv;
        if (!ires)
            return dsv;

        ComPtr<ID3D11Texture2D> tex;
        ires.As(&tex);

        D3D11_TEXTURE2D_DESC tex_dec = {};
        tex->GetDesc(&tex_dec);

        if (tex_dec.Format == DXGI_FORMAT_R32_TYPELESS) // TODO
        {
            D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
            dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
            dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
            dsv_desc.Texture2DArray.ArraySize = tex_dec.ArraySize;
            ASSERT_SUCCEEDED(device->CreateDepthStencilView(tex.Get(), &dsv_desc, &dsv));
        }
        else
        {
            ASSERT_SUCCEEDED(device->CreateDepthStencilView(tex.Get(), nullptr, &dsv));
        }
        return dsv;
    }

    ComPtr<ID3D11RenderTargetView> ToRtv(const ComPtr<IUnknown>& ires)
    {
        ComPtr<ID3D11RenderTargetView> rtv;
        if (!ires)
            return rtv;

        ComPtr<ID3D11Resource> res;
        ires.As(&res);
        ASSERT_SUCCEEDED(device->CreateRenderTargetView(res.Get(), nullptr, &rtv));
        return rtv;
    }

    virtual void ResizeBackBuffer(int width, int height) override;
    ComPtr<IDXGISwapChain3> swap_chain;
};
