#include "LightPass.h"

LightPass::LightPass(Context & context, Input & input, int width, int height)
    : m_context(context)
    , m_input(input)
    , m_width(width)
    , m_height(height)
    , m_program(context.device)
{
}

void LightPass::OnUpdate()
{
    float angle = 0.0;
    float light_r = 2.5;
    glm::vec3 light_pos = glm::vec3(light_r * cos(angle), 25.0f, light_r * sin(angle));
    glm::vec3 cameraPosView = glm::vec3(glm::vec4(m_input.camera.GetCameraPos(), 1.0));
    glm::vec3 lightPosView = glm::vec3(glm::vec4(light_pos, 1.0));

    m_program.ps.cbuffer.ConstantBuffer.lightPos = glm::vec4(lightPosView, 0);
    m_program.ps.cbuffer.ConstantBuffer.viewPos = glm::vec4(cameraPosView, 0.0);
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
        m_context.device_context->DrawIndexed(cur_mesh.indices.size(), 0, 0);
    }
}

void LightPass::OnResize(int width, int height)
{
}
