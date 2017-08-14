#include "DXSample.h"
#include <chrono>
#include <SOIL.h>
#include <Utility.h>
#include <FileUtility.h>

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
    CreateGeometry();

    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    HRESULT hr = d3d11Device->CreateSamplerState(&sampDesc, &textureSamplerState);
}

void DXSample::OnUpdate()
{
    static std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
    static std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
    static float angle = 0.0;

    end = std::chrono::system_clock::now();
    int64_t elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    start = std::chrono::system_clock::now();

    if (use_rotare)
        angle += elapsed / 2e6f;

    float z_width = (m_modelOfFile->bound_box.z_max - m_modelOfFile->bound_box.z_min);
    float y_width = (m_modelOfFile->bound_box.y_max - m_modelOfFile->bound_box.y_min);
    float x_width = (m_modelOfFile->bound_box.y_max - m_modelOfFile->bound_box.y_min);
    float model_width = (z_width + y_width + x_width) / 3.0f;
    float scale = 256.0f / std::max(z_width, x_width);
    model_width *= scale;

    float offset_x = (m_modelOfFile->bound_box.x_max + m_modelOfFile->bound_box.x_min) / 2.0f;
    float offset_y = (m_modelOfFile->bound_box.y_max + m_modelOfFile->bound_box.y_min) / 2.0f;
    float offset_z = (m_modelOfFile->bound_box.z_max + m_modelOfFile->bound_box.z_min) / 2.0f;

    cameraPosition = glm::vec3(0.0f, model_width * 0.25f, -model_width * 2.0f);
    cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::mat4 model = glm::rotate(-angle, glm::vec3(0.0, 1.0, 0.0)) * glm::translate(glm::vec3(-offset_x, -offset_y, -offset_z)) * glm::scale(glm::vec3(scale, scale, scale));
    glm::mat4 view = glm::lookAtRH(cameraPosition, cameraTarget, cameraUp);
    glm::mat4 proj = glm::perspectiveFovRH(45.0f * (3.14f / 180.0f), (float)m_width, (float)m_height, 0.1f, 1024.0f);

    cbPerObject.model = glm::transpose(model);
    cbPerObject.view = glm::transpose(view);
    cbPerObject.projection = glm::transpose(proj);
    cbPerObject.lightPos = glm::vec4(cameraPosition, 0.0);
    cbPerObject.viewPos = glm::vec4(cameraPosition, 0.0);

    d3d11DevCon->UpdateSubresource(cbPerObjectBuffer.Get(), 0, NULL, &cbPerObject, 0, 0);

    d3d11DevCon->VSSetConstantBuffers(0, 1, cbPerObjectBuffer.GetAddressOf());
}

void DXSample::OnRender()
{
    float bgColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    d3d11DevCon->ClearRenderTargetView(renderTargetView.Get(), bgColor);

    d3d11DevCon->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    d3d11DevCon->PSSetSamplers(0, 1, &textureSamplerState);

    for (size_t mesh_id = 0; mesh_id < m_modelOfFile->meshes.size(); ++mesh_id)
    {
        DX11Mesh & cur_mesh = m_modelOfFile->meshes[mesh_id];

        UINT stride = sizeof(cur_mesh.vertices[0]);
        UINT offset = 0;
        d3d11DevCon->IASetVertexBuffers(0, 1, cur_mesh.vertBuffer.GetAddressOf(), &stride, &offset);
        d3d11DevCon->IASetIndexBuffer(cur_mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

        std::vector<ID3D11ShaderResourceView*> use_textures(5, nullptr);
        static uint32_t frameId = 0;
        for (size_t i = 0; i < cur_mesh.textures.size(); ++i)
        {
            uint32_t texture_slot = 0;
            switch (cur_mesh.textures[i].type)
            {
            case aiTextureType_DIFFUSE:
                texture_slot = 0;
                break;
            case aiTextureType_HEIGHT:
                texture_slot = 1;
                break;
            case aiTextureType_SPECULAR:
                texture_slot = 2;
                break;
            case aiTextureType_SHININESS:
                texture_slot = 3;
                break;
            case aiTextureType_AMBIENT:
                texture_slot = 4;
                break;
            default:
                continue;
            }

            use_textures[texture_slot] = cur_mesh.texResources[i].Get();
        }

        d3d11DevCon->PSSetShaderResources(0, use_textures.size(), use_textures.data());

        d3d11DevCon->DrawIndexed(cur_mesh.indices.size(), 0, 0);
    }

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


    //Describe our Depth/Stencil Buffer

    D3D11_TEXTURE2D_DESC depthStencilDesc;
    depthStencilDesc.Width = m_width;
    depthStencilDesc.Height = m_height;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.ArraySize = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilDesc.CPUAccessFlags = 0;
    depthStencilDesc.MiscFlags = 0;


    hr = d3d11Device->CreateTexture2D(&depthStencilDesc, NULL, &depthStencilBuffer);
    hr = d3d11Device->CreateDepthStencilView(depthStencilBuffer.Get(), NULL, &depthStencilView);

    d3d11DevCon->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), depthStencilView.Get());
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

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DX11Mesh::Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DX11Mesh::Vertex, normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(DX11Mesh::Vertex, texCoords), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DX11Mesh::Vertex, tangent), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DX11Mesh::Vertex, bitangent), D3D11_INPUT_PER_VERTEX_DATA, 0 },
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

    D3D11_BUFFER_DESC cbbd;
    ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));

    cbbd.Usage = D3D11_USAGE_DEFAULT;
    cbbd.ByteWidth = sizeof(cbPerObject);
    cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbbd.CPUAccessFlags = 0;
    cbbd.MiscFlags = 0;

    hr = d3d11Device->CreateBuffer(&cbbd, NULL, &cbPerObjectBuffer);

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
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    //Set the Viewport
    d3d11DevCon->RSSetViewports(1, &viewport);
}

void DXSample::CreateGeometry()
{
    m_modelOfFile.reset(new Model<DX11Mesh>("model/export3dcoat/export3dcoat.obj"));
     
    for (DX11Mesh & cur_mesh : m_modelOfFile->meshes)
    {
        cur_mesh.SetupMesh(d3d11Device, d3d11DevCon);
    }

    for (DX11Mesh & cur_mesh : m_modelOfFile->meshes)
    {
        for (size_t i = 0; i < cur_mesh.textures.size(); ++i)
        {
            auto metadata = LoadImageDataFromFile(cur_mesh.textures[i].path);

            ComPtr<ID3D11Texture2D> resource;

            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width = static_cast<UINT>(metadata.textureWidth);
            desc.Height = static_cast<UINT>(metadata.textureHeight);
            desc.MipLevels = static_cast<UINT>(1);
            desc.ArraySize = static_cast<UINT>(1);
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = 0;

            D3D11_SUBRESOURCE_DATA textureBufferData;
            ZeroMemory(&textureBufferData, sizeof(textureBufferData));
            textureBufferData.pSysMem = metadata.imageData.get();
            textureBufferData.SysMemPitch = metadata.bytesPerRow; // size of all our triangle vertex data
            textureBufferData.SysMemSlicePitch = metadata.bytesPerRow * metadata.textureHeight; // also the size of our triangle vertex data

            HRESULT hr = d3d11Device->CreateTexture2D(&desc, &textureBufferData, &resource);

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;

            hr = d3d11Device->CreateShaderResourceView(resource.Get(), &srvDesc, &cur_mesh.texResources[i]);
        }
    }
}

DXSample::TexInfo DXSample::LoadImageDataFromFile(std::string filename)
{
    TexInfo texInfo = {};

    unsigned char *image = SOIL_load_image(filename.c_str(), &texInfo.textureWidth, &texInfo.textureHeight, 0, SOIL_LOAD_RGBA);

    DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    texInfo.numBitsPerPixel = BitsPerPixel(dxgiFormat); // number of bits per pixel
    texInfo.bytesPerRow = (texInfo.textureWidth * texInfo.numBitsPerPixel) / 8; // number of bytes in each row of the image data
    texInfo.imageSize = texInfo.bytesPerRow * texInfo.textureHeight; // total image size in bytes

    texInfo.imageData.reset(new uint8_t[texInfo.imageSize]);
    memcpy(texInfo.imageData.get(), image, texInfo.imageSize);
    SOIL_free_image_data(image);

    return texInfo;
}