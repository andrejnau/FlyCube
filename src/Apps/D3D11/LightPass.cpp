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
}

void LightPass::OnUpdate()
{
    m_program.ps.cbuffer.ConstantBuffer.lightPos = m_input.light_pos;
    m_program.ps.cbuffer.ConstantBuffer.viewPos = m_input.camera.GetCameraPos();



    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 Down = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 Left = glm::vec3(-1.0f, 0.0f, 0.0f);
    glm::vec3 Right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 ForwardRH = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 ForwardLH = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 BackwardRH = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 BackwardLH = glm::vec3(0.0f, 0.0f, -1.0f);

    glm::vec3 position = m_input.light_pos;

    m_program.ps.cbuffer.Params.Projection = glm::transpose(glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 1024.0f));

    std::array<glm::mat4, 6>& view = m_program.ps.cbuffer.Params.View;
    view[0] = glm::transpose(glm::lookAt(position, position + Right, Up));
    view[1] = glm::transpose(glm::lookAt(position, position + Left, Up));
    view[2] = glm::transpose(glm::lookAt(position, position + Up, BackwardRH));
    view[3] = glm::transpose(glm::lookAt(position, position + Down, ForwardRH));
    view[4] = glm::transpose(glm::lookAt(position, position + BackwardLH, Up));
    view[5] = glm::transpose(glm::lookAt(position, position + ForwardLH, Up));
}

void LightPass::OnRender()
{
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
        m_context.device_context->PSSetShaderResources(m_program.ps.texture.cubeShadowMap, 1, m_input.shadow_pass.srv.GetAddressOf());
        m_context.device_context->PSSetShaderResources(m_program.ps.texture.cubeShadowMap + 1, 1, m_input.shadow_pass.cubemap_id.GetAddressOf());
        m_context.device_context->DrawIndexed(cur_mesh.indices.size(), 0, 0);
    }
}

void LightPass::OnResize(int width, int height)
{
}
