#include "DXSample.h"
#include <Utilities/DXUtility.h>
#include <Utilities/FileUtility.h>
#include <Utilities/State.h>
#include <D3Dcompiler.h>
#include <GLFW/glfw3.h>
#include <chrono>

DXSample::DXSample()
    : m_width(0)
    , m_height(0)
{}

DXSample::~DXSample()
{}

IScene::Ptr DXSample::Create()
{
    return std::make_unique<DXSample>();
}

void DXSample::OnInit(int width, int height)
{
    m_width = width;
    m_height = height;

    CreateDeviceAndSwapChain();

    m_shader_geometry_pass.reset(new DXShader(m_device, "shaders/DX11/GeometryPass_VS.hlsl", "shaders/DX11/GeometryPass_PS.hlsl"));
    m_shader_light_pass.reset(new DXShader(m_device, "shaders/DX11/LightPass_VS.hlsl", "shaders/DX11/LightPass_PS.hlsl"));

    CreateRT();
    CreateViewPort();
    CreateSampler();
    m_model_of_file = CreateGeometry("model/sponza/sponza.obj");
    m_model_square = CreateGeometry("model/square.obj");
    InitGBuffer();
    m_camera.SetViewport(m_width, m_height);

    m_device_context->PSSetSamplers(0, 1, m_texture_sampler.GetAddressOf());
    m_device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void DXSample::OnUpdate()
{
    UpdateCameraMovement();
    UpdateAngle();

    auto& constant_buffer_geometry_pass = m_shader_geometry_pass->GetVSCBuffer("ConstantBuffer");
    auto& constant_buffer_light_pass = m_shader_light_pass->GetPSCBuffer("ConstantBuffer");
    UpdateCBuffers(constant_buffer_geometry_pass, constant_buffer_light_pass);
    constant_buffer_geometry_pass.Update(m_device_context);
    constant_buffer_light_pass.Update(m_device_context);
}

void DXSample::OnRender()
{
    GeometryPass();
    LightPass();
    ASSERT_SUCCEEDED(m_swap_chain->Present(0, 0));
}

void DXSample::GeometryPass()
{
    m_device_context->RSSetViewports(1, &m_viewport);
    m_device_context->VSSetShader(m_shader_geometry_pass->vertex_shader.Get(), 0, 0);
    m_device_context->PSSetShader(m_shader_geometry_pass->pixel_shader.Get(), 0, 0);

    m_shader_geometry_pass->GetVSCBuffer("ConstantBuffer").SetOnPipeline(m_device_context);
    m_shader_geometry_pass->GetPSCBuffer("TexturesEnables").SetOnPipeline(m_device_context);
    m_shader_geometry_pass->GetPSCBuffer("Material").SetOnPipeline(m_device_context);
    m_shader_geometry_pass->GetPSCBuffer("Light").SetOnPipeline(m_device_context);
    m_device_context->IASetInputLayout(m_shader_geometry_pass->input_layout.Get());

    std::vector<ID3D11RenderTargetView*> rtvs = {
        m_position_rtv.Get(),
        m_normal_rtv.Get(),
        m_ambient_rtv.Get(),
        m_diffuse_rtv.Get(),
        m_specular_rtv.Get(),
    };

    float bgColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    for (auto & rtv : rtvs)
    {
        m_device_context->ClearRenderTargetView(rtv, bgColor);
    }
    m_device_context->ClearDepthStencilView(m_depth_stencil_view.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_device_context->OMSetRenderTargets(rtvs.size(), rtvs.data(), m_depth_stencil_view.Get());

    auto& light = m_shader_geometry_pass->GetPSCBuffer("Light");
    SetLight(light);
    light.Update(m_device_context);

    for (DX11Mesh& cur_mesh : m_model_of_file->meshes)
    {
        UINT stride = sizeof(cur_mesh.vertices[0]);
        UINT offset = 0;
        m_device_context->IASetVertexBuffers(0, 1, cur_mesh.vertBuffer.GetAddressOf(), &stride, &offset);
        m_device_context->IASetIndexBuffer(cur_mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

        auto& textures_enables = m_shader_geometry_pass->GetPSCBuffer("TexturesEnables");
        std::vector<int> use_textures = MapTextures(cur_mesh, textures_enables);
        for (int i = 0; i < use_textures.size(); ++i)
        {
            int tex_id = use_textures[i];
            ComPtr<ID3D11ShaderResourceView> srv;
            if (tex_id >= 0)
                srv = cur_mesh.texResources[tex_id];
            m_device_context->PSSetShaderResources(i, 1, srv.GetAddressOf());
        }
        textures_enables.Update(m_device_context);

        auto& material = m_shader_geometry_pass->GetPSCBuffer("Material");
        SetMaterial(material, cur_mesh);
        material.Update(m_device_context);

        m_device_context->DrawIndexed(cur_mesh.indices.size(), 0, 0);
    }
}

void DXSample::LightPass()
{
    m_device_context->VSSetShader(m_shader_light_pass->vertex_shader.Get(), 0, 0);
    m_device_context->PSSetShader(m_shader_light_pass->pixel_shader.Get(), 0, 0);
    m_shader_light_pass->GetPSCBuffer("ConstantBuffer").SetOnPipeline(m_device_context);
    m_device_context->IASetInputLayout(m_shader_light_pass->input_layout.Get());

    m_device_context->OMSetRenderTargets(1, m_render_target_view.GetAddressOf(), m_depth_stencil_view.Get());

    float bgColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_device_context->ClearRenderTargetView(m_render_target_view.Get(), bgColor);
    m_device_context->ClearDepthStencilView(m_depth_stencil_view.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    for (size_t mesh_id = 0; mesh_id < m_model_square->meshes.size(); ++mesh_id)
    {
        DX11Mesh & cur_mesh = m_model_square->meshes[mesh_id];

        UINT stride = sizeof(cur_mesh.vertices[0]);
        UINT offset = 0;
        m_device_context->IASetVertexBuffers(0, 1, cur_mesh.vertBuffer.GetAddressOf(), &stride, &offset);
        m_device_context->IASetIndexBuffer(cur_mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

        std::vector<ID3D11ShaderResourceView*> use_textures =
        {
            m_position_srv.Get(),
            m_normal_srv.Get(),
            m_ambient_srv.Get(),
            m_diffuse_srv.Get(),
            m_specular_srv.Get()
        };

        m_device_context->PSSetShaderResources(0, use_textures.size(), use_textures.data());
        m_device_context->DrawIndexed(cur_mesh.indices.size(), 0, 0);
    }
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

        m_render_target_view.Reset();

        DXGI_SWAP_CHAIN_DESC desc = {};
        ASSERT_SUCCEEDED(m_swap_chain->GetDesc(&desc));
        ASSERT_SUCCEEDED(m_swap_chain->ResizeBuffers(1, width, height, desc.BufferDesc.Format, desc.Flags));

        CreateRT();
        CreateViewPort();
        InitGBuffer();
        m_camera.SetViewport(m_width, m_height);
    }
}

inline glm::mat4 DXSample::StoreMatrix(const glm::mat4 & m)
{
    return glm::transpose(m);
}

void DXSample::CreateDeviceAndSwapChain()
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

void DXSample::CreateRT()
{
    //Create our BackBuffer
    ComPtr<ID3D11Texture2D> BackBuffer;
    ASSERT_SUCCEEDED(m_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer));

    //Create our Render Target
    ASSERT_SUCCEEDED(m_device->CreateRenderTargetView(BackBuffer.Get(), nullptr, &m_render_target_view));

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

    ASSERT_SUCCEEDED(m_device->CreateTexture2D(&depthStencilDesc, nullptr, &m_depth_stencil_buffer));
    ASSERT_SUCCEEDED(m_device->CreateDepthStencilView(m_depth_stencil_buffer.Get(), nullptr, &m_depth_stencil_view));
}

void DXSample::CreateViewPort()
{
    ZeroMemory(&m_viewport, sizeof(D3D11_VIEWPORT));
    m_viewport.TopLeftX = 0;
    m_viewport.TopLeftY = 0;
    m_viewport.Width = m_width;
    m_viewport.Height = m_height;
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;
}

void DXSample::CreateSampler()
{
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    ASSERT_SUCCEEDED(m_device->CreateSamplerState(&sampDesc, &m_texture_sampler));
}

std::unique_ptr<Model<DX11Mesh>> DXSample::CreateGeometry(const std::string& path)
{
    std::unique_ptr<Model<DX11Mesh>> model = std::make_unique<Model<DX11Mesh>>(path);
     
    for (DX11Mesh & cur_mesh : model->meshes)
    {
        cur_mesh.SetupMesh(m_device, m_device_context);
    } 
    return model;
}

void DXSample::CreateRTV(ComPtr<ID3D11RenderTargetView>& rtv, ComPtr<ID3D11ShaderResourceView>& srv)
{
    D3D11_TEXTURE2D_DESC texture_desc;
    ZeroMemory(&texture_desc, sizeof(texture_desc));
    texture_desc.Width = m_width;
    texture_desc.Height = m_height;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    ComPtr<ID3D11Texture2D> texture;
    ASSERT_SUCCEEDED(m_device->CreateTexture2D(&texture_desc, nullptr, &texture));

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    
    ASSERT_SUCCEEDED(m_device->CreateShaderResourceView(texture.Get(), &srvDesc, &srv));

    ASSERT_SUCCEEDED(m_device->CreateRenderTargetView(texture.Get(), nullptr, &rtv));
}

void DXSample::InitGBuffer()
{
    CreateRTV(m_position_rtv, m_position_srv);
    CreateRTV(m_normal_rtv, m_normal_srv);
    CreateRTV(m_ambient_rtv, m_ambient_srv);
    CreateRTV(m_diffuse_rtv, m_diffuse_srv);
    CreateRTV(m_specular_rtv, m_specular_srv);
}
