#include "DXSample.h"
#include "Utility.h"
#include "FileUtility.h"

#include <SOIL.h>

#include <chrono>

DXSample::DXSample(int width, int height)
    : m_width(width)
    , m_height(height)
{}

DXSample::~DXSample()
{}

void DXSample::OnInit()
{
    InitializeDirect3d11App();
    InitScene();
}

void DXSample::OnUpdate()
{
    static std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
    static std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
    static float angle = 0.0;

    end = std::chrono::system_clock::now();
    int64_t elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    start = std::chrono::system_clock::now();
}

void DXSample::OnRender()
{
    float bgColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    d3d11DevCon->ClearRenderTargetView(renderTargetView.Get(), bgColor);

    d3d11DevCon->Draw(3, 0);

    SwapChain->Present(0, 0);
}

void DXSample::OnDestroy()
{
}

void DXSample::OnSizeChanged(int width, int height)
{
    if (width != m_width || height != m_height)
    {
        m_width = width;
        m_height = height;

        renderTargetView.Reset();

        DXGI_SWAP_CHAIN_DESC desc = {};
        SwapChain->GetDesc(&desc);
        HRESULT hr = SwapChain->ResizeBuffers(1, width, height, desc.BufferDesc.Format, desc.Flags);

        CreateRT();
        CreateViewPort();
    }
}

UINT DXSample::GetWidth() const
{
    return m_width;
}

UINT DXSample::GetHeight() const
{
    return m_height;
}

bool DXSample::InitializeDirect3d11App()
{
    HRESULT hr;

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
    swapChainDesc.OutputWindow = Win32Application::GetHwnd();
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;


    //Create our SwapChain
    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, NULL, NULL,
        D3D11_SDK_VERSION, &swapChainDesc, &SwapChain, &d3d11Device, NULL, &d3d11DevCon);


    CreateRT();
    return true;
}

void DXSample::CreateRT()
{
    HRESULT hr;

    //Create our BackBuffer
    ID3D11Texture2D* BackBuffer;
    hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer);

    //Create our Render Target
    hr = d3d11Device->CreateRenderTargetView(BackBuffer, NULL, &renderTargetView);
    BackBuffer->Release();

    //Set our Render Target
    d3d11DevCon->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), NULL);
}

bool DXSample::InitScene()
{
    HRESULT hr;
    //Compile Shaders from shader file

    ComPtr<ID3DBlob> pErrors;
    hr = D3DCompileFromFile(
        GetAssetFullPath(L"shaders/DX11/VertexShader.hlsl").c_str(),
        nullptr,
        nullptr,
        "main",
        "vs_5_0",
        0,
        0,
        &VS_Buffer,
        &pErrors);

    hr = D3DCompileFromFile(
        GetAssetFullPath(L"shaders/DX11/PixelShader.hlsl").c_str(),
        nullptr,
        nullptr,
        "main",
        "ps_5_0",
        0,
        0,
        &PS_Buffer,
        &pErrors);

    //Create the Shader Objects
    hr = d3d11Device->CreateVertexShader(VS_Buffer->GetBufferPointer(), VS_Buffer->GetBufferSize(), NULL, &VS);
    hr = d3d11Device->CreatePixelShader(PS_Buffer->GetBufferPointer(), PS_Buffer->GetBufferSize(), NULL, &PS);

    //Set Vertex and Pixel Shaders
    d3d11DevCon->VSSetShader(VS.Get(), 0, 0);
    d3d11DevCon->PSSetShader(PS.Get(), 0, 0);

    //Create the vertex buffer
    Vertex v[] =
    {
        {Vector3(0.0f, 0.5f, 0.5f), Vector4(1.0f, 0.0f, 0.0f, 1.0f)},
        {Vector3(0.5f, -0.5f, 0.5f), Vector4(0.0f, 1.0f, 0.0f, 1.0f)},
        {Vector3(-0.5f, -0.5f, 0.5f), Vector4(0.0f, 0.0f, 1.0f, 1.0f)}
    };

    D3D11_BUFFER_DESC vertexBufferDesc;
    ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = sizeof(Vertex) * 3;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    vertexBufferDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA vertexBufferData;

    ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
    vertexBufferData.pSysMem = v;
    hr = d3d11Device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, triangleVertBuffer.GetAddressOf());

    //Set the vertex buffer
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    d3d11DevCon->IASetVertexBuffers(0, 1, triangleVertBuffer.GetAddressOf(), &stride, &offset);

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE(layout);

    //Create the Input Layout
    hr = d3d11Device->CreateInputLayout(layout, numElements, VS_Buffer->GetBufferPointer(),
        VS_Buffer->GetBufferSize(), &vertLayout);

    //Set the Input Layout
    d3d11DevCon->IASetInputLayout(vertLayout.Get());

    //Set Primitive Topology
    d3d11DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

   
    CreateViewPort();
    return true;
}

void DXSample::CreateViewPort()
{
    //Create the Viewport
    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = m_width;
    viewport.Height = m_height;

    //Set the Viewport
    d3d11DevCon->RSSetViewports(1, &viewport);
}