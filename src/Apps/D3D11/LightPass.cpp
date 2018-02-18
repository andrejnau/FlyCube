#include "LightPass.h"
#include "DX11CreateUtils.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

LightPass::LightPass(Context& context, const Input& input, int width, int height)
    : m_context(context)
    , m_input(input)
    , m_width(width)
    , m_height(height)
    , m_program(context, std::bind(&LightPass::SetDefines, this, std::placeholders::_1))
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
    m_context.device_context->PSSetSamplers(m_program.ps.sampler.LightCubeShadowComparsionSampler, 1, m_shadow_sampler.GetAddressOf());

    D3D11_RASTERIZER_DESC shadowState;
    ZeroMemory(&shadowState, sizeof(D3D11_RASTERIZER_DESC));
    shadowState.FillMode = D3D11_FILL_SOLID;
    shadowState.CullMode = D3D11_CULL_BACK;
    shadowState.DepthBias = 4096;
    m_context.device->CreateRasterizerState(&shadowState, &m_rasterizer_state);

    m_input.camera.SetCameraPos(glm::vec3(-3.0, 2.75, 0.0));
    m_input.camera.SetCameraYaw(-178.0f);
    m_input.camera.SetCameraYaw(-1.75f);

    CreateRtvSrv(m_context, 1, width, height, m_rtv, output.srv);
    CreateDsv(m_context, 1, width, height, m_depth_stencil_view);
}

void LightPass::SetDefines(Program<LightPassPS, LightPassVS>& program)
{
    program.ps.define["SAMPLE_COUNT"] = std::to_string(m_settings.msaa_count);
}

void LightPass::OnUpdate()
{
    m_program.ps.cbuffer.ConstantBuffer.lightPos = m_input.light_pos;
    m_program.ps.cbuffer.ConstantBuffer.viewPos = m_input.camera.GetCameraPos();

    m_program.ps.cbuffer.ShadowParams.s_near = m_settings.s_near;
    m_program.ps.cbuffer.ShadowParams.s_far = m_settings.s_far;
    m_program.ps.cbuffer.ShadowParams.s_size = m_settings.s_size;
    m_program.ps.cbuffer.ShadowParams.use_shadow = true;
}

void LightPass::OnRender()
{
    ID3D11RasterizerState* cur = nullptr;
    m_context.device_context->RSGetState(&cur);
    m_context.device_context->RSSetState(m_rasterizer_state.Get());

    m_program.UseProgram();
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

        m_program.ps.srv.gPosition.Attach(m_input.geometry_pass.position_srv);
        m_program.ps.srv.gNormal.Attach(m_input.geometry_pass.normal_srv);
        m_program.ps.srv.gAmbient.Attach(m_input.geometry_pass.ambient_srv);
        m_program.ps.srv.gDiffuse.Attach(m_input.geometry_pass.diffuse_srv);
        m_program.ps.srv.gSpecular.Attach(m_input.geometry_pass.specular_srv);
        m_program.ps.srv.LightCubeShadowMap.Attach(m_input.shadow_pass.srv);
        m_context.device_context->DrawIndexed(cur_mesh.indices.size(), 0, 0);
    }

    m_context.device_context->OMSetRenderTargets(0, nullptr, nullptr);

    std::vector<ID3D11ShaderResourceView*> empty(D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
    m_context.device_context->PSSetShaderResources(0, empty.size(), empty.data());
}

void LightPass::OnResize(int width, int height)
{
    m_width = width;
    m_height = height;
    CreateRtvSrv(m_context, 1, width, height, m_rtv, output.srv);
    CreateDsv(m_context, 1, width, height, m_depth_stencil_view);
}

void LightPass::OnModifySettings(const Settings& settings)
{
    Settings prev = m_settings;
    m_settings = settings;
    if (prev.msaa_count != m_settings.msaa_count)
    {
        m_program.ps.define["SAMPLE_COUNT"] = std::to_string(m_settings.msaa_count);
        m_program.ps.UpdateShader();
    }
}
