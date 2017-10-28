#include "DX11Context/DX11Context.h"
#include "DX11Program/DX11Program.h"
#include "DX11Geometry/DX11Geometry.h"

#include <Utilities/DXUtility.h>


DX11Context::Ptr CreareDX11Context(int width, int height)
{
    return DX11Context::Ptr(new DX11Context(width, height));
}

FrameBuffer::Ptr DX11Context::CreateFrameBuffer()
{
    return FrameBuffer::Ptr();
}

Geometry::Ptr DX11Context::CreateGeometry(const std::string& path)
{
    auto model = std::make_shared<Model<DX11Mesh>>(path);
    for (DX11Mesh & cur_mesh : model->meshes)
    {
        cur_mesh.SetupMesh(m_device, m_device_context);
    }
    return model;
}

Program::Ptr DX11Context::CreateProgram(const std::string & vertex, const std::string & pixel)
{
    return std::make_shared<DX11Program>(m_device, vertex, pixel);
}

SwapChain::Ptr DX11Context::CreateSwapChain()
{
    return SwapChain::Ptr();
}

Texture::Ptr DX11Context::CreateTexture()
{
    return Texture::Ptr();
}

void DX11Context::Draw(const Geometry::Ptr & geometry)
{
}

void DX11Context::CreateDeviceAndSwapChain()
{
    //Describe our Buffer
    DXGI_MODE_DESC bufferDesc;
    ZeroMemory(&bufferDesc, sizeof(DXGI_MODE_DESC));
    bufferDesc.Width = m_width;
    bufferDesc.Height = m_height;
    bufferDesc.RefreshRate.Numerator = 60;
    bufferDesc.RefreshRate.Denominator = 1;
    bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    //Describe our SwapChain
    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
    swapChainDesc.BufferDesc = bufferDesc;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 1;
    swapChainDesc.OutputWindow = GetActiveWindow();
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    DWORD create_device_flags = 0;
#if 0
    create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    ASSERT_SUCCEEDED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, create_device_flags, nullptr, 0,
        D3D11_SDK_VERSION, &swapChainDesc, &m_swap_chain, &m_device, nullptr, &m_device_context));
}