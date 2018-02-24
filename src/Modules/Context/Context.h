#pragma once

#include <wrl.h>
#include <GLFW/glfw3.h>
#include <dxgiformat.h>
using namespace Microsoft::WRL;

enum BindFlag
{
    kRtv = 1 << 1,
    kDsv = 1 << 2,
    kSrv = 1 << 3,
};

class Context
{
public:
    Context(GLFWwindow* window, int width, int height);
    virtual ComPtr<IUnknown> GetBackBuffer() = 0;
    virtual void Present() = 0;
    virtual void DrawIndexed(UINT IndexCount) = 0;
    virtual void OnResize(int width, int height);
    virtual void SetViewport(int width, int height) = 0;
    GLFWwindow* window;

    virtual ComPtr<IUnknown> CreateTexture(uint32_t bind_flag, DXGI_FORMAT format, uint32_t msaa_count, int width, int height, int depth = 1) = 0;
    virtual ComPtr<IUnknown> CreateBufferSRV(void* data, size_t size, size_t stride) = 0;
    virtual ComPtr<IUnknown> CreateSamplerAnisotropic() = 0;
    virtual ComPtr<IUnknown> CreateSamplerShadow() = 0;
protected:
    virtual void ResizeBackBuffer(int width, int height) = 0;
    int m_width;
    int m_height;
};
