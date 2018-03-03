#pragma once

#include <wrl.h>
#include <GLFW/glfw3.h>
#include <dxgiformat.h>
#include <memory>
#include <Program/ProgramApi.h>
using namespace Microsoft::WRL;

#include "Context/BaseTypes.h"
#include "Context/Resource.h"

class Context
{
public:
    virtual Resource::Ptr CreateTexture(uint32_t bind_flag, DXGI_FORMAT format, uint32_t msaa_count, int width, int height, int depth = 1, int mip_levels = 1) = 0;
    virtual Resource::Ptr CreateBuffer(uint32_t bind_flag, UINT buffer_size, size_t stride) = 0;
    virtual void UpdateSubresource(const Resource::Ptr& ires, UINT DstSubresource, const void *pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch, size_t version = 0) = 0;
    
    Context(GLFWwindow* window, int width, int height);
    virtual Resource::Ptr GetBackBuffer() = 0;
    virtual void Present(const Resource::Ptr& ires) = 0;
    virtual void DrawIndexed(UINT IndexCount) = 0;
    virtual void Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ) = 0;
    virtual void OnResize(int width, int height);
    virtual void SetViewport(int width, int height) = 0;
    virtual void OMSetRenderTargets(std::vector<Resource::Ptr> rtv, Resource::Ptr dsv) = 0;
    virtual void ClearRenderTarget(Resource::Ptr rtv, const FLOAT ColorRGBA[4]) = 0;
    virtual void ClearDepthStencil(Resource::Ptr dsv, UINT ClearFlags, FLOAT Depth, UINT8 Stencil) = 0;

    virtual void IASetIndexBuffer(Resource::Ptr res, UINT SizeInBytes, DXGI_FORMAT Format) = 0;
    virtual void IASetVertexBuffer(UINT slot, Resource::Ptr res, UINT SizeInBytes, UINT Stride) = 0;

    virtual void BeginEvent(LPCWSTR Name) = 0;
    virtual void EndEvent() = 0;

    GLFWwindow* window;

    virtual std::unique_ptr<ProgramApi> CreateProgram() = 0;
  
protected:
    virtual void ResizeBackBuffer(int width, int height) = 0;
    int m_width;
    int m_height;
};
