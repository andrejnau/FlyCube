#include "LightPass.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

LightPass::LightPass(Context& context, const Input& input, int width, int height)
    : m_context(context)
    , m_input(input)
    , m_width(width)
    , m_height(height)
    , m_program(context.device, std::bind(&LightPass::SetDefines, this, std::placeholders::_1))
{
    D3D11_SAMPLER_DESC samp_desc = {};
    samp_desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
    samp_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp_desc.ComparisonFunc = D3D11_COMPARISON_LESS;
    samp_desc.MinLOD = 0;
    samp_desc.MaxLOD = D3D11_FLOAT32_MAX;

    ASSERT_SUCCEEDED(m_context.device->CreateSamplerState(&samp_desc, &m_shadow_sampler));
    m_context.device_context->PSSetSamplers(1, 1, m_shadow_sampler.GetAddressOf());

    D3D11_RASTERIZER_DESC shadowState;
    ZeroMemory(&shadowState, sizeof(D3D11_RASTERIZER_DESC));
    shadowState.FillMode = D3D11_FILL_SOLID;
    shadowState.CullMode = D3D11_CULL_BACK;
    shadowState.DepthBias = 4096;
    m_context.device->CreateRasterizerState(&shadowState, &m_rasterizer_state);

    m_input.camera.SetCameraPos(glm::vec3(0.0, 2.75, 0.0));
    m_input.camera.SetCameraYaw(-178.0f);
    m_input.camera.SetCameraYaw(-1.75f);

    CreateRtvSrv(m_rtv, output.srv);
    CreateDsv();
}

void LightPass::SetDefines(Program<LightPassPS, LightPassVS>& program)
{
    program.ps.define["SAMPLE_COUNT"] = std::to_string(m_settings.msaa_count);
}

void LightPass::OnUpdate()
{
    m_program.ps.cbuffer.ConstantBuffer.lightPos = m_input.camera.GetCameraPos();//m_input.light_pos;
    m_program.ps.cbuffer.ConstantBuffer.viewPos = m_input.camera.GetCameraPos();

    m_program.ps.cbuffer.ShadowParams.s_near = m_settings.s_near;
    m_program.ps.cbuffer.ShadowParams.s_far = m_settings.s_far;
    m_program.ps.cbuffer.ShadowParams.s_size = m_settings.s_size;
    m_program.ps.cbuffer.ShadowParams.use_shadow = false;
}

void LightPass::OnRender()
{
    ID3D11RasterizerState* cur = nullptr;
    m_context.device_context->RSGetState(&cur);
    m_context.device_context->RSSetState(m_rasterizer_state.Get());

    m_program.UseProgram(m_context.device_context);
    m_context.device_context->IASetInputLayout(m_program.vs.input_layout.Get());

    float color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_context.device_context->ClearRenderTargetView(m_rtv.Get(), color);
    m_context.device_context->ClearDepthStencilView(m_depth_stencil_view.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_context.device_context->OMSetRenderTargets(1, m_rtv.GetAddressOf(), m_depth_stencil_view.Get());

    for (DX11Mesh& cur_mesh : m_input.model.meshes)
    {
        cur_mesh.indices_buffer.Bind();
        cur_mesh.positions_buffer.BindToSlot(m_program.vs.geometry.POSITION);
        cur_mesh.texcoords_buffer.BindToSlot(m_program.vs.geometry.TEXCOORD);

        m_context.device_context->PSSetShaderResources(m_program.ps.texture.gPosition, 1, m_input.geometry_pass.position_srv.GetAddressOf());
        m_context.device_context->PSSetShaderResources(m_program.ps.texture.gNormal, 1, m_input.geometry_pass.normal_srv.GetAddressOf());
        m_context.device_context->PSSetShaderResources(m_program.ps.texture.gAmbient, 1, m_input.geometry_pass.ambient_srv.GetAddressOf());
        m_context.device_context->PSSetShaderResources(m_program.ps.texture.gDiffuse, 1, m_input.geometry_pass.diffuse_srv.GetAddressOf());
        m_context.device_context->PSSetShaderResources(m_program.ps.texture.gSpecular, 1, m_input.geometry_pass.specular_srv.GetAddressOf());
        m_context.device_context->PSSetShaderResources(m_program.ps.texture.LightCubeShadowMap, 1, m_input.shadow_pass.srv.GetAddressOf());
        m_context.device_context->DrawIndexed(cur_mesh.indices.size(), 0, 0);
    }

    m_context.device_context->OMSetRenderTargets(0, nullptr, nullptr);

    std::vector<ID3D11ShaderResourceView*> empty(m_program.ps.texture.LightCubeShadowMap);
    m_context.device_context->PSSetShaderResources(0, empty.size(), empty.data());
}

void LightPass::OnResize(int width, int height)
{
    CreateRtvSrv(m_rtv, output.srv);
    CreateDsv();
}

void LightPass::OnModifySettings(const Settings& settings)
{
    Settings prev = m_settings;
    m_settings = settings;
    if (prev.msaa_count != m_settings.msaa_count)
    {
        m_program.ps.define["SAMPLE_COUNT"] = std::to_string(m_settings.msaa_count);
        m_program.ps.UpdateShader(m_context.device);
    }
}

void LightPass::CreateRtvSrv(ComPtr<ID3D11RenderTargetView>& rtv, ComPtr<ID3D11ShaderResourceView>& srv)
{
    D3D11_TEXTURE2D_DESC texture_desc = {};
    texture_desc.Width = m_width;
    texture_desc.Height = m_height;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;

    ComPtr<ID3D11Texture2D> texture;
    ASSERT_SUCCEEDED(m_context.device->CreateTexture2D(&texture_desc, nullptr, &texture));

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srv_desc.Texture2D.MipLevels = 1;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

    ASSERT_SUCCEEDED(m_context.device->CreateShaderResourceView(texture.Get(), &srv_desc, &srv));
    ASSERT_SUCCEEDED(m_context.device->CreateRenderTargetView(texture.Get(), nullptr, &rtv));
}

void LightPass::CreateDsv()
{
    D3D11_TEXTURE2D_DESC depth_stencil_desc = {};
    depth_stencil_desc.Width = m_width;
    depth_stencil_desc.Height = m_height;
    depth_stencil_desc.MipLevels = 1;
    depth_stencil_desc.ArraySize = 1;
    depth_stencil_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depth_stencil_desc.Usage = D3D11_USAGE_DEFAULT;
    depth_stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depth_stencil_desc.SampleDesc.Count = 1;
    depth_stencil_desc.SampleDesc.Quality = 0;

    ComPtr<ID3D11Texture2D> depth_stencil_buffer;
    ASSERT_SUCCEEDED(m_context.device->CreateTexture2D(&depth_stencil_desc, nullptr, &depth_stencil_buffer));
    ASSERT_SUCCEEDED(m_context.device->CreateDepthStencilView(depth_stencil_buffer.Get(), nullptr, &m_depth_stencil_view));
}
