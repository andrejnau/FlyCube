#include "LightPass.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

LightPass::LightPass(Context& context, const Input& input, int width, int height)
    : m_context(context)
    , m_input(input)
    , m_width(width)
    , m_height(height)
    , m_program(context.device)
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
    shadowState.DepthBias = 10000;
    m_context.device->CreateRasterizerState(&shadowState, &m_rasterizer_state);

    D3D_SHADER_MACRO define[] = { "USE_MSAA", "1", NULL, NULL };
    m_program.ApplyDefine(m_context.device, m_settings.msaa_count > 1 ? define : nullptr);
}

void LightPass::OnUpdate()
{
    m_program.ps.cbuffer.ConstantBuffer.lightPos = m_input.light_pos;
    m_program.ps.cbuffer.ConstantBuffer.viewPos = m_input.camera.GetCameraPos();
    m_program.ps.cbuffer.Params.sample_count = m_settings.msaa_count;

    m_program.ps.cbuffer.ShadowParams.s_near = 0.1;
    m_program.ps.cbuffer.ShadowParams.s_far = 1024.0;
    m_program.ps.cbuffer.ShadowParams.s_size = 2048;
}

void LightPass::OnRender()
{
    ID3D11RasterizerState* cur = nullptr;
    m_context.device_context->RSGetState(&cur);
    m_context.device_context->RSSetState(m_rasterizer_state.Get());

    m_program.UseProgram(m_context.device_context);
    m_context.device_context->IASetInputLayout(m_program.vs.input_layout.Get());

    float color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_context.device_context->ClearRenderTargetView(m_input.render_target_view.Get(), color);
    m_context.device_context->ClearDepthStencilView(m_input.depth_stencil_view.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_context.device_context->OMSetRenderTargets(1, m_input.render_target_view.GetAddressOf(), m_input.depth_stencil_view.Get());

    for (DX11Mesh& cur_mesh : m_input.model.meshes)
    {
        cur_mesh.SetIndexBuffer();
        cur_mesh.SetVertexBuffer(m_program.vs.geometry.POSITION, VertexType::kPosition);
        cur_mesh.SetVertexBuffer(m_program.vs.geometry.TEXCOORD, VertexType::kTexcoord);

        m_context.device_context->PSSetShaderResources(m_program.ps.texture.gPosition, 1, m_input.geometry_pass.position_srv.GetAddressOf());
        m_context.device_context->PSSetShaderResources(m_program.ps.texture.gNormal, 1, m_input.geometry_pass.normal_srv.GetAddressOf());
        m_context.device_context->PSSetShaderResources(m_program.ps.texture.gAmbient, 1, m_input.geometry_pass.ambient_srv.GetAddressOf());
        m_context.device_context->PSSetShaderResources(m_program.ps.texture.gDiffuse, 1, m_input.geometry_pass.diffuse_srv.GetAddressOf());
        m_context.device_context->PSSetShaderResources(m_program.ps.texture.gSpecular, 1, m_input.geometry_pass.specular_srv.GetAddressOf());
        m_context.device_context->PSSetShaderResources(m_program.ps.texture.LightCubeShadowMap, 1, m_input.shadow_pass.srv.GetAddressOf());
        m_context.device_context->DrawIndexed(cur_mesh.indices.size(), 0, 0);
    }

    std::vector<ID3D11ShaderResourceView*> empty(m_program.ps.texture.LightCubeShadowMap);
    m_context.device_context->PSSetShaderResources(0, empty.size(), empty.data());
}

void LightPass::OnResize(int width, int height)
{
}

void LightPass::OnModifySettings(const Settings& settings)
{
    Settings prev = m_settings;
    m_settings = settings;
    if (prev.msaa_count != settings.msaa_count)
    {
        D3D_SHADER_MACRO define[] = { "USE_MSAA", "1", NULL, NULL };
        m_program.ApplyDefine(m_context.device, settings.msaa_count > 1 ? define : nullptr);
    }
}
