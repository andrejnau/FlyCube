#include "DX11Scene.h"
#include <Utilities/DXUtility.h>
#include <Utilities/FileUtility.h>
#include <Utilities/State.h>
#include <D3Dcompiler.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <glm/gtx/transform.hpp>

DX11Scene::DX11Scene(int width, int height)
    : m_width(width)
    , m_height(height)
    , m_context(m_width, m_height)
    , m_shader_light_pass(m_context.device)
    , m_model_of_file("model/sponza/sponza.obj")
    , m_model_square("model/square.obj")
    , m_geometry_pass(m_context, m_model_of_file,  m_camera, width, height)
{
    for (DX11Mesh& cur_mesh : m_model_of_file.meshes)
        cur_mesh.SetupMesh(m_context);
    for (DX11Mesh& cur_mesh : m_model_square.meshes)
        cur_mesh.SetupMesh(m_context);

    CreateRT();
    CreateViewPort();
    CreateSampler();

    m_camera.SetViewport(m_width, m_height);
    m_context.device_context->RSSetViewports(1, &m_viewport);
    m_context.device_context->PSSetSamplers(0, 1, m_texture_sampler.GetAddressOf());
    m_context.device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

IScene::Ptr DX11Scene::Create(int width, int height)
{
    return std::make_unique<DX11Scene>(width, height);
}

void DX11Scene::OnUpdate()
{
    UpdateCameraMovement();
    UpdateAngle();

    m_geometry_pass.OnUpdate();

    float light_r = 2.5;
    glm::vec3 light_pos = glm::vec3(light_r * cos(m_angle), 25.0f, light_r * sin(m_angle));
    glm::vec3 cameraPosView = glm::vec3(glm::vec4(m_camera.GetCameraPos(), 1.0));
    glm::vec3 lightPosView = glm::vec3(glm::vec4(light_pos, 1.0));

    m_shader_light_pass.ps.cbuffer.ConstantBuffer.lightPos = glm::vec4(lightPosView, 0);
    m_shader_light_pass.ps.cbuffer.ConstantBuffer.viewPos = glm::vec4(cameraPosView, 0.0);
}

void DX11Scene::OnRender()
{
    m_geometry_pass.OnRender();
    LightPass();
    ASSERT_SUCCEEDED(m_context.swap_chain->Present(0, 0));
}

void DX11Scene::LightPass()
{
    m_shader_light_pass.UseProgram(m_context.device_context);
    m_context.device_context->IASetInputLayout(m_shader_light_pass.vs.input_layout.Get());

    float bgColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_context.device_context->ClearRenderTargetView(m_render_target_view.Get(), bgColor);
    m_context.device_context->ClearDepthStencilView(m_depth_stencil_view.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_context.device_context->OMSetRenderTargets(1, m_render_target_view.GetAddressOf(), m_depth_stencil_view.Get());

    for (DX11Mesh& cur_mesh : m_model_square.meshes)
    {
        m_context.device_context->IASetIndexBuffer(cur_mesh.indices_buffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        cur_mesh.SetVertexBuffer(m_context, m_shader_light_pass.vs.geometry.POSITION, VertexType::kPosition);
        cur_mesh.SetVertexBuffer(m_context, m_shader_light_pass.vs.geometry.TEXCOORD, VertexType::kTexcoord);

        m_context.device_context->PSSetShaderResources(m_shader_light_pass.ps.texture.gPosition, 1, m_geometry_pass.m_position_srv.GetAddressOf());
        m_context.device_context->PSSetShaderResources(m_shader_light_pass.ps.texture.gNormal,   1, m_geometry_pass.m_normal_srv.GetAddressOf());
        m_context.device_context->PSSetShaderResources(m_shader_light_pass.ps.texture.gAmbient,  1, m_geometry_pass.m_ambient_srv.GetAddressOf());
        m_context.device_context->PSSetShaderResources(m_shader_light_pass.ps.texture.gDiffuse,  1, m_geometry_pass.m_diffuse_srv.GetAddressOf());
        m_context.device_context->PSSetShaderResources(m_shader_light_pass.ps.texture.gSpecular, 1, m_geometry_pass.m_specular_srv.GetAddressOf());
        m_context.device_context->DrawIndexed(cur_mesh.indices.size(), 0, 0);
    }
}

void DX11Scene::OnResize(int width, int height)
{
    if (width == m_width && height == m_height)
        return;

    m_width = width;
    m_height = height;

    m_render_target_view.Reset();

    DXGI_SWAP_CHAIN_DESC desc = {};
    ASSERT_SUCCEEDED(m_context.swap_chain->GetDesc(&desc));
    ASSERT_SUCCEEDED(m_context.swap_chain->ResizeBuffers(1, width, height, desc.BufferDesc.Format, desc.Flags));

    CreateRT();
    CreateViewPort();
    m_camera.SetViewport(m_width, m_height);

    m_geometry_pass.OnResize(width, height);
}

void DX11Scene::CreateRT()
{
    ComPtr<ID3D11Texture2D> back_buffer;
    ASSERT_SUCCEEDED(m_context.swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back_buffer));
    ASSERT_SUCCEEDED(m_context.device->CreateRenderTargetView(back_buffer.Get(), nullptr, &m_render_target_view));

    D3D11_TEXTURE2D_DESC depth_stencil_desc = {};
    depth_stencil_desc.Width = m_width;
    depth_stencil_desc.Height = m_height;
    depth_stencil_desc.MipLevels = 1;
    depth_stencil_desc.ArraySize = 1;
    depth_stencil_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depth_stencil_desc.SampleDesc.Count = 1;
    depth_stencil_desc.Usage = D3D11_USAGE_DEFAULT;
    depth_stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ASSERT_SUCCEEDED(m_context.device->CreateTexture2D(&depth_stencil_desc, nullptr, &m_depth_stencil_buffer));
    ASSERT_SUCCEEDED(m_context.device->CreateDepthStencilView(m_depth_stencil_buffer.Get(), nullptr, &m_depth_stencil_view));
}

void DX11Scene::CreateViewPort()
{
    ZeroMemory(&m_viewport, sizeof(D3D11_VIEWPORT));
    m_viewport.TopLeftX = 0;
    m_viewport.TopLeftY = 0;
    m_viewport.Width = m_width;
    m_viewport.Height = m_height;
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;
}

void DX11Scene::CreateSampler()
{
    D3D11_SAMPLER_DESC samp_desc = {};
    samp_desc.Filter = D3D11_FILTER_ANISOTROPIC;
    samp_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samp_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samp_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samp_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samp_desc.MinLOD = 0;
    samp_desc.MaxLOD = D3D11_FLOAT32_MAX;

    ASSERT_SUCCEEDED(m_context.device->CreateSamplerState(&samp_desc, &m_texture_sampler));
}
