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
    virtual Resource::Ptr CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, uint32_t stride) override;
    virtual void UpdateSubresource(const Resource::Ptr& ires, uint32_t DstSubresource, const void *pSrcData, uint32_t SrcRowPitch, uint32_t SrcDepthPitch) override;

    virtual void SetViewport(float width, float height) override;
    virtual void SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom) override;

    virtual void IASetIndexBuffer(Resource::Ptr res, uint32_t SizeInBytes, DXGI_FORMAT Format) override;
    virtual void IASetVertexBuffer(uint32_t slot, Resource::Ptr res, uint32_t SizeInBytes, uint32_t Stride) override;

    virtual void BeginEvent(LPCWSTR Name) override;
    virtual void EndEvent() override;

    virtual void DrawIndexed(uint32_t IndexCount, uint32_t StartIndexLocation, INT BaseVertexLocation) override;
    virtual void Dispatch(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ) override;

    virtual Resource::Ptr GetBackBuffer() override;
    virtual void Present(const Resource::Ptr& ires) override;

    void UseProgram(DX11ProgramApi& program_api);

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> device_context;

private:
    virtual void ResizeBackBuffer(int width, int height) override;
    ComPtr<ID3DUserDefinedAnnotation> perf;
    DX11ProgramApi* m_current_program = nullptr;
    ComPtr<IDXGISwapChain3> m_swap_chain;
};
