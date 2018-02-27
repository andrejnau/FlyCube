#include "LightPass.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

LightPass::LightPass(Context& context, const Input& input, int width, int height)
    : m_context(context)
    , m_input(input)
    , m_width(width)
    , m_height(height)
    , m_program(context, std::bind(&LightPass::SetDefines, this, std::placeholders::_1))
{
    m_shadow_sampler = m_context.CreateSamplerShadow();
    m_g_sampler = m_context.CreateSamplerAnisotropic();

    m_input.camera.SetCameraPos(glm::vec3(-3.0, 2.75, 0.0));
    m_input.camera.SetCameraYaw(-178.0f);
    m_input.camera.SetCameraYaw(-1.75f);

    if (m_input.rtv)
        m_input.rtv = m_context.CreateTexture(BindFlag::kRtv | BindFlag::kSrv, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, m_width, m_height, 1);
    m_depth_stencil_view = m_context.CreateTexture(BindFlag::kDsv, DXGI_FORMAT_D24_UNORM_S8_UINT, 1, m_width, m_height, 1);
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
    m_program.ps.cbuffer.ShadowParams.use_shadow = false;
    m_program.ps.cbuffer.ShadowParams.use_occlusion = 0*m_settings.use_occlusion;
}

void LightPass::OnRender()
{
    m_context.SetViewport(m_width, m_height);

    m_program.UseProgram();

    m_program.ps.sampler.g_sampler.Attach(m_g_sampler);
    m_program.ps.sampler.LightCubeShadowComparsionSampler.Attach(m_shadow_sampler);

    float color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_context.OMSetRenderTargets({ m_input.rtv }, m_depth_stencil_view);
    m_context.ClearRenderTarget(m_input.rtv, color);
    m_context.ClearDepthStencil(m_depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    for (DX11Mesh& cur_mesh : m_input.model.meshes)
    {
        cur_mesh.indices_buffer.Bind();
        cur_mesh.positions_buffer.BindToSlot(m_program.vs.ia.POSITION);
        cur_mesh.texcoords_buffer.BindToSlot(m_program.vs.ia.TEXCOORD);

        m_program.ps.srv.gPosition.Attach(m_input.geometry_pass.position);
        m_program.ps.srv.gNormal.Attach(m_input.geometry_pass.normal);
        m_program.ps.srv.gAmbient.Attach(m_input.geometry_pass.ambient);
        m_program.ps.srv.gDiffuse.Attach(m_input.geometry_pass.diffuse);
        m_program.ps.srv.gSpecular.Attach(m_input.geometry_pass.specular);
        m_program.ps.srv.LightCubeShadowMap.Attach(m_input.shadow_pass.srv);
        m_program.ps.srv.gSSAO.Attach(m_input.ssao_pass.srv_blur);
        m_context.DrawIndexed(cur_mesh.indices.size());
    }

    m_context.OMSetRenderTargets({}, nullptr);
}

void LightPass::OnResize(int width, int height)
{
    m_width = width;
    m_height = height;

    if (m_input.rtv)
        m_input.rtv = m_context.CreateTexture(BindFlag::kRtv | BindFlag::kSrv, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, m_width, m_height, 1);
    m_depth_stencil_view = m_context.CreateTexture(BindFlag::kDsv, DXGI_FORMAT_D24_UNORM_S8_UINT, 1, m_width, m_height, 1);
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
