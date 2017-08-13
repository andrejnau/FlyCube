#pragma once

#include "IDXSample.h"
#include "Win32Application.h"
#include "Geometry.h"
#include "Util.h"

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

    void CreateGeometry();

    struct TexInfo
    {
        //D3D12_RESOURCE_DESC resourceDescription;
        std::unique_ptr<uint8_t[]> imageData;
        int textureWidth;
        int textureHeight;
        int numBitsPerPixel;
        int imageSize;
        int bytesPerRow;
    };

    DXSample::TexInfo LoadImageDataFromFile(std::string filename);

    ComPtr<IDXGISwapChain> SwapChain;
    ComPtr<ID3D11Device> d3d11Device;
    ComPtr<ID3D11DeviceContext> d3d11DevCon;
    ComPtr<ID3D11RenderTargetView> renderTargetView;

    ComPtr<ID3D11DepthStencilView> depthStencilView;
    ComPtr<ID3D11Texture2D> depthStencilBuffer;

    ComPtr<ID3D11VertexShader> VS;
    ComPtr<ID3D11PixelShader> PS;
    ComPtr<ID3DBlob> VS_Buffer;
    ComPtr<ID3DBlob> PS_Buffer;
    ComPtr<ID3D11InputLayout> vertLayout;

    std::unique_ptr<Model> m_modelOfFile;

    int m_width;
    int m_height;


    const bool use_rotare = true;

    Matrix cameraProjMat; // this will store our projection matrix
    Matrix cameraViewMat; // this will store our view matrix
    Matrix cubeWorldMat; // our first cubes world matrix (transformation matrix)

    Vector4 cameraPosition; // this is our cameras position vector
    Vector4 cameraTarget; // a vector describing the point in space our camera is looking at
    Vector4 cameraUp; // the worlds up vector

    ComPtr<ID3D11Buffer> cbPerObjectBuffer;

    struct ConstantBufferPerObject
    {
        Matrix model;
        Matrix view;
        Matrix projection;
        Vector4 lightPos;
        Vector4 viewPos;
    };

    ConstantBufferPerObject cbPerObject;


    ComPtr<ID3D11SamplerState> textureSamplerState;
};