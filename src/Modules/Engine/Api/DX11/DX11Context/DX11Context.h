#pragma once

#include <memory>
#include <Context/Context.h>

#include <d3d11.h>
#include <DXGI1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>

using namespace Microsoft::WRL;
using namespace DirectX;

class DX11Context : public Context
{
public:
    using Ptr = std::shared_ptr<DX11Context>;
    int m_width;
    int m_height;

    DX11Context(int width, int height)
        : m_width(width)
        , m_height(height)
    {
        CreateDeviceAndSwapChain();
    }

    virtual FrameBuffer::Ptr CreateFrameBuffer() override;
    virtual Geometry::Ptr CreateGeometry(const std::string& path) override;
    virtual Program::Ptr CreateProgram(const std::string& vertex, const std::string& pixel) override;
    virtual SwapChain::Ptr CreateSwapChain() override;
    virtual Texture::Ptr CreateTexture() override;

    virtual void Draw(const Geometry::Ptr& geometry) override;


    void CreateDeviceAndSwapChain();

    ID3D11Device* GetDevice()
    {
        return m_device.Get();
    }

    ID3D11DeviceContext* GetDeviceContext()
    {
        return m_device_context.Get();
    }

    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_device_context;
    ComPtr<IDXGISwapChain> m_swap_chain;
};

DX11Context::Ptr CreareDX11Context(int width, int height);
