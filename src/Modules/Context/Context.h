#pragma once

#include <wrl.h>
#include <dxgiformat.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <array>

#include <Program/ProgramApi.h>
#include "Context/BaseTypes.h"
#include "Context/Resource.h"

using namespace Microsoft::WRL;

class Context
{
public:
    Context(GLFWwindow* window, int width, int height);

    virtual std::unique_ptr<ProgramApi> CreateProgram() = 0;
    virtual Resource::Ptr CreateTexture(uint32_t bind_flag, DXGI_FORMAT format, uint32_t msaa_count, int width, int height, int depth = 1, int mip_levels = 1) = 0;
    virtual Resource::Ptr CreateBuffer(uint32_t bind_flag, UINT buffer_size, size_t stride) = 0;
    virtual void UpdateSubresource(const Resource::Ptr& ires, UINT DstSubresource, const void *pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch) = 0;

    virtual void SetViewport(int width, int height) = 0;
    virtual void SetScissorRect(LONG left, LONG top, LONG right, LONG bottom) = 0;

    virtual void IASetIndexBuffer(Resource::Ptr res, UINT SizeInBytes, DXGI_FORMAT Format) = 0;
    virtual void IASetVertexBuffer(UINT slot, Resource::Ptr res, UINT SizeInBytes, UINT Stride) = 0;

    virtual void BeginEvent(LPCWSTR Name) = 0;
    virtual void EndEvent() = 0;

    virtual void DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation) = 0;
    virtual void Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ) = 0;

    virtual Resource::Ptr GetBackBuffer() = 0;
    virtual void Present(const Resource::Ptr& ires) = 0;

    void OnResize(int width, int height);
    size_t GetFrameIndex() const;
    GLFWwindow* GetWindow();

    static constexpr size_t FrameCount = 3;
  
protected:
    virtual void ResizeBackBuffer(int width, int height) = 0;
    int m_width;
    int m_height;
    GLFWwindow* m_window;
    uint32_t m_frame_index = 0;
};

template <typename T>
class PerFrameData
{
public:
    PerFrameData(Context& context)
        : m_context(context)
    {
    }

    T& get()
    {
        return m_data[m_context.GetFrameIndex()];
    }

    T& operator[](size_t pos)
    {
        return m_data[pos];
    }
private:
    Context& m_context;
    std::array<T, Context::FrameCount> m_data;
};
