#pragma once

#include "IDXSample.h"
#include "Win32Application.h"

#include <memory>
#include <string>

#include <d3d11.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <SimpleMath.h>

#include <DXGI1_4.h>

#include <wrl/client.h>
using namespace Microsoft::WRL;

using namespace DirectX;
using namespace DirectX::SimpleMath;

class DXSample : public IDXSample
{
public:
    DXSample(int width, int height);
    ~DXSample();

    virtual void OnInit() override;
    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnDestroy() override;
    virtual void OnSizeChanged(int width, int height) override;

    UINT GetWidth() const  override;
    UINT GetHeight() const  override;

private:

    bool InitializeDirect3d11App();
    void CreateRT();
    bool InitScene();

    void CreateViewPort();

    ComPtr<IDXGISwapChain> SwapChain;
    ComPtr<ID3D11Device> d3d11Device;
    ComPtr<ID3D11DeviceContext> d3d11DevCon;
    ComPtr<ID3D11RenderTargetView> renderTargetView;

    //The vertex Structure
    struct Vertex
    {
        Vertex(float x, float y, float z, float r, float g, float b, float a)
            : pos(x, y, z)
            , color(r, g, b, a)
        {}
        Vector3 pos;
        Vector4 color;
    };

    ComPtr<ID3D11Buffer> squareVertBuffer;
    ComPtr<ID3D11Buffer> squareIndexBuffer;

    ComPtr<ID3D11VertexShader> VS;
    ComPtr<ID3D11PixelShader> PS;
    ComPtr<ID3DBlob> VS_Buffer;
    ComPtr<ID3DBlob> PS_Buffer;
    ComPtr<ID3D11InputLayout> vertLayout;

    int m_width;
    int m_height;
};