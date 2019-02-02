#pragma once

#include <d3d11_4.h>
#include <dxgi1_4.h>
#include <wrl.h>

#include "Context/Context.h"
#include <Resource/DX11Resource.h>

using namespace Microsoft::WRL;

class DX11ProgramApi;

class DX11Context : public Context
{
public:
    DX11Context(GLFWwindow* window);

    virtual std::unique_ptr<ProgramApi> CreateProgram() override;
    virtual Resource::Ptr CreateTexture(uint32_t bind_flag, gli::format format, uint32_t msaa_count, int width, int height, int depth = 1, int mip_levels = 1) override;
    virtual Resource::Ptr CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, uint32_t stride) override;
    virtual Resource::Ptr CreateSampler(const SamplerDesc& desc) override;
    virtual void UpdateSubresource(const Resource::Ptr& ires, uint32_t DstSubresource, const void *pSrcData, uint32_t SrcRowPitch, uint32_t SrcDepthPitch) override;

    virtual void SetViewport(float width, float height) override;
    virtual void SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom) override;

    virtual void IASetIndexBuffer(Resource::Ptr res, gli::format Format) override;
    virtual void IASetVertexBuffer(uint32_t slot, Resource::Ptr res) override;

    virtual void BeginEvent(const std::string& name) override;
    virtual void EndEvent() override;

    virtual void DrawIndexed(uint32_t IndexCount, uint32_t StartIndexLocation, int32_t BaseVertexLocation) override;
    virtual void Dispatch(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ) override;

    virtual Resource::Ptr GetBackBuffer() override;
    virtual void Present() override;

    void UseProgram(DX11ProgramApi& program_api);

    ComPtr<ID3D11Device4> device;
    ComPtr<ID3D11DeviceContext4> device_context;

private:
    virtual void ResizeBackBuffer(int width, int height) override;
    ComPtr<ID3DUserDefinedAnnotation> perf;
    DX11ProgramApi* m_current_program = nullptr;
    ComPtr<IDXGISwapChain3> m_swap_chain;
};
