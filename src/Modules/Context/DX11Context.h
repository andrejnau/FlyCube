#pragma once

#include <d3d11_1.h>
#include <dxgi1_4.h>
#include <wrl.h>

#include "Context/Context.h"
#include "Context/DX11Resource.h"

using namespace Microsoft::WRL;

class DX11ProgramApi;

class DX11Context : public Context
{
public:
    DX11Context(GLFWwindow* window, int width, int height);

    virtual std::unique_ptr<ProgramApi> CreateProgram() override;
    virtual Resource::Ptr CreateTexture(uint32_t bind_flag, DXGI_FORMAT format, uint32_t msaa_count, int width, int height, int depth = 1, int mip_levels = 1) override;
    virtual Resource::Ptr CreateBuffer(uint32_t bind_flag, UINT buffer_size, size_t stride) override;
    virtual void UpdateSubresource(const Resource::Ptr& ires, UINT DstSubresource, const void *pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch) override;

    virtual void SetViewport(int width, int height) override;
    virtual void SetScissorRect(LONG left, LONG top, LONG right, LONG bottom) override;

    virtual void IASetIndexBuffer(Resource::Ptr res, UINT SizeInBytes, DXGI_FORMAT Format) override;
    virtual void IASetVertexBuffer(UINT slot, Resource::Ptr res, UINT SizeInBytes, UINT Stride) override;

    virtual void BeginEvent(LPCWSTR Name) override;
    virtual void EndEvent() override;

    virtual void DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation) override;
    virtual void Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ) override;

    virtual Resource::Ptr GetBackBuffer() override;
    virtual void Present(const Resource::Ptr& ires) override;

    void UseProgram(DX11ProgramApi& program_api);

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> device_context;

private:
    virtual void ResizeBackBuffer(int width, int height) override;
    ComPtr<ID3DUserDefinedAnnotation> perf;
    DX11ProgramApi* current_program = nullptr;
    ComPtr<IDXGISwapChain3> m_swap_chain;
};
