#pragma once

#include <wrl.h>
#include <GLFW/glfw3.h>
#include <dxgiformat.h>
#include <memory>
#include <Program/ProgramApi.h>
using namespace Microsoft::WRL;

#include "Context/BaseTypes.h"

class Context
{
public:
    Context(GLFWwindow* window, int width, int height);
    virtual ComPtr<IUnknown> GetBackBuffer() = 0;
    virtual void Present() = 0;
    virtual void DrawIndexed(UINT IndexCount) = 0;
    virtual void Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ) = 0;
    virtual void OnResize(int width, int height);
    virtual void SetViewport(int width, int height) = 0;
    virtual void OMSetRenderTargets(std::vector<ComPtr<IUnknown>> rtv, ComPtr<IUnknown> dsv) = 0;
    virtual void ClearRenderTarget(ComPtr<IUnknown> rtv, const FLOAT ColorRGBA[4]) = 0;
    virtual void ClearDepthStencil(ComPtr<IUnknown> dsv, UINT ClearFlags, FLOAT Depth, UINT8 Stencil) = 0;

    virtual void IASetIndexBuffer(ComPtr<IUnknown> res, UINT SizeInBytes, DXGI_FORMAT Format) = 0;
    virtual void IASetVertexBuffer(UINT slot, ComPtr<IUnknown> res, UINT SizeInBytes, UINT Stride) = 0;

    virtual void BeginEvent(LPCWSTR Name) = 0;
    virtual void EndEvent() = 0;

    GLFWwindow* window;

    virtual ComPtr<IUnknown> CreateTexture(uint32_t bind_flag, DXGI_FORMAT format, uint32_t msaa_count, int width, int height, int depth = 1, int mip_levels = 1) = 0;

    virtual ComPtr<IUnknown> CreateShadowRSState() = 0;
    virtual void RSSetState(ComPtr<IUnknown> state) = 0;
    virtual std::unique_ptr<ProgramApi> CreateProgram() = 0;
    virtual ComPtr<IUnknown> CreateBuffer(uint32_t bind_flag, UINT buffer_size, size_t stride, const std::string& name) = 0;
    virtual void UpdateSubresource(ComPtr<IUnknown> ires, UINT DstSubresource, const void *pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch) = 0;
  
protected:
    virtual void ResizeBackBuffer(int width, int height) = 0;
    int m_width;
    int m_height;
};
