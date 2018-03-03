#pragma once

#include "Context/Context.h"
#include <d3d11.h>
#include <d3d11_1.h>
#include <DXGI1_4.h>
#include <wrl.h>
#include <GLFW/glfw3.h>
#include <vector>

using namespace Microsoft::WRL;

#include "Context/Resource.h"

class DX11Context : public Context
{
public:
    virtual Resource::Ptr CreateTexture(uint32_t bind_flag, DXGI_FORMAT format, uint32_t msaa_count, int width, int height, int depth = 1, int mip_levels = 1) override;
    virtual Resource::Ptr CreateBuffer(uint32_t bind_flag, UINT buffer_size, size_t stride) override;
    virtual void UpdateSubresource(const Resource::Ptr& ires, UINT DstSubresource, const void *pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch, size_t version = 0) override;

    DX11Context(GLFWwindow* window, int width, int height);
    virtual Resource::Ptr GetBackBuffer() override;
    virtual void Present(const Resource::Ptr& ires) override;
    virtual void DrawIndexed(UINT IndexCount) override;
    virtual void Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ) override;
    virtual void SetViewport(int width, int height) override;
    virtual void OMSetRenderTargets(std::vector<Resource::Ptr> rtv, Resource::Ptr dsv) override;
    virtual void ClearRenderTarget(Resource::Ptr rtv, const FLOAT ColorRGBA[4]) override;
    virtual void ClearDepthStencil(Resource::Ptr dsv, UINT ClearFlags, FLOAT Depth, UINT8 Stencil) override;

    virtual std::unique_ptr<ProgramApi> CreateProgram() override;

    virtual void IASetIndexBuffer(Resource::Ptr res, UINT SizeInBytes, DXGI_FORMAT Format) override;
    virtual void IASetVertexBuffer(UINT slot, Resource::Ptr res, UINT SizeInBytes, UINT Stride) override;

    virtual void BeginEvent(LPCWSTR Name) override;
    virtual void EndEvent() override;

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> device_context;
    ComPtr<ID3DUserDefinedAnnotation> perf;
private:
    ComPtr<ID3D11DepthStencilView> ToDsv(const Resource::Ptr& ires)
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
            ASSERT_SUCCEEDED(device->CreateDepthStencilView(tex.Get(), &dsv_desc, &dsv));
        }
        else
        {
            ASSERT_SUCCEEDED(device->CreateDepthStencilView(tex.Get(), nullptr, &dsv));
        }
        return dsv;
    }

    ComPtr<ID3D11RenderTargetView> ToRtv(const Resource::Ptr& ires)
    {
        auto res = std::static_pointer_cast<DX11Resource>(ires);

        ComPtr<ID3D11RenderTargetView> rtv;
        if (!ires)
            return rtv;

        ASSERT_SUCCEEDED(device->CreateRenderTargetView(res->resource.Get(), nullptr, &rtv));
        return rtv;
    }

    virtual void ResizeBackBuffer(int width, int height) override;
    ComPtr<IDXGISwapChain3> swap_chain;
};
