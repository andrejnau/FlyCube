#pragma once

#include <d3d11.h>
#include <DXGI1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <memory>
#include <string>
#include <IDXSample.h>
#include <Win32Application.h>
#include <Util.h>
#include "Geometry.h"
#include "DX11Geometry.h"
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

using namespace Microsoft::WRL;
using namespace DirectX;

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

    std::unique_ptr<Model<DX11Mesh>> m_modelOfFile;

    int m_width;
    int m_height;

    const bool use_rotare = true;

    glm::vec3 cameraPosition; // this is our cameras position vector
    glm::vec3 cameraTarget; // a vector describing the point in space our camera is looking at
    glm::vec3 cameraUp; // the worlds up vector

    ComPtr<ID3D11Buffer> cbPerObjectBuffer;

    struct ConstantBufferPerObject
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
        glm::vec4 lightPos;
        glm::vec4 viewPos;
    };

    ConstantBufferPerObject cbPerObject;


    ComPtr<ID3D11SamplerState> textureSamplerState;
};