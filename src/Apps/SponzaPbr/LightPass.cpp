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
    m_input.camera.SetCameraPos(glm::vec3(-3.0, 2.75, 0.0));
    m_input.camera.SetCameraYaw(-178.0f);
    m_input.camera.SetCameraYaw(-1.75f);

    CreateSizeDependentResources();
    m_sampler = m_context.CreateSampler({
        SamplerFilter::kAnisotropic,
        SamplerTextureAddressMode::kWrap,
        SamplerComparisonFunc::kNever });

    m_sampler_brdf = m_context.CreateSampler({
        SamplerFilter::kMinMagMipLinear,
        SamplerTextureAddressMode::kClamp,
        SamplerComparisonFunc::kNever });
}

void LightPass::SetDefines(Program<LightPassPS, LightPassVS>& program)
{
    program.ps.define["SAMPLE_COUNT"] = std::to_string(m_settings.msaa_count);
}

void LightPass::OnUpdate()
{
    glm::vec3 camera_position = m_input.camera.GetCameraPos();

    m_program.ps.cbuffer.Light.viewPos = glm::vec4(camera_position, 0.0);
    m_program.ps.cbuffer.Settings.use_ssao = m_settings.use_ssao;
    m_program.ps.cbuffer.Settings.use_ao = m_settings.use_ao;
    m_program.ps.cbuffer.Settings.use_IBL_diffuse = m_settings.use_IBL_diffuse;
    m_program.ps.cbuffer.Settings.use_IBL_specular = m_settings.use_IBL_specular;
    m_program.ps.cbuffer.Settings.only_ambient = m_settings.only_ambient;
    m_program.ps.cbuffer.Settings.ambient_power = m_settings.ambient_power;
    m_program.ps.cbuffer.Settings.use_spec_ao_by_ndotv_roughness = m_settings.use_spec_ao_by_ndotv_roughness;

    if (m_settings.light_in_camera)
    {
        m_program.ps.cbuffer.Light.light_pos[0] = glm::vec4(camera_position, 0);
        m_program.ps.cbuffer.Light.light_color[0] = m_settings.light_power * glm::vec4(1, 1, 1, 0.0);
        for (size_t i = 1; i < std::size(m_program.ps.cbuffer.Light.light_pos); ++i)
        {
            m_program.ps.cbuffer.Light.light_color[i] = glm::vec4(0);
        }
    }
    else
    {
        int i = 0;
        for (int x = -13; x <= 13; ++x)
        {
            int q = 1;
            for (int z = -1; z <= 1; ++z)
            {
                if (i < std::size(m_program.ps.cbuffer.Light.light_pos))
                {
                    m_program.ps.cbuffer.Light.light_pos[i] = glm::vec4(x, 1.5, z - 0.33, 0);
                    float color = 0.0;
                    if (m_settings.use_white_ligth)
                        color = 1;
                    m_program.ps.cbuffer.Light.light_color[i] = m_settings.light_power * glm::vec4(q == 1 ? 1 : color, q == 2 ? 1 : color, q == 3 ? 1 : color, 0.0);
                    ++i;
                    ++q;
                }
            }
        }
    }

    size_t cnt = 0;
    for (auto& cur_mesh : m_input.model.ia.ranges)
        ++cnt;
    m_program.SetMaxEvents(cnt);
}

void LightPass::OnRender()
{
    m_context.SetViewport(m_width, m_height);

    m_program.UseProgram();

    m_program.ps.sampler.g_sampler.Attach(m_sampler);
    m_program.ps.sampler.brdf_sampler.Attach(m_sampler_brdf);

    float color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_program.ps.om.rtv0.Attach(output.rtv).Clear(color);
    m_program.ps.om.dsv.Attach(m_depth_stencil_view).Clear(D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_input.model.ia.indices.Bind();
    m_input.model.ia.positions.BindToSlot(m_program.vs.ia.POSITION);
    m_input.model.ia.texcoords.BindToSlot(m_program.vs.ia.TEXCOORD);

    for (auto& range : m_input.model.ia.ranges)
    {
        m_program.ps.srv.gPosition.Attach(m_input.geometry_pass.position);
        m_program.ps.srv.gNormal.Attach(m_input.geometry_pass.normal);
        m_program.ps.srv.gAlbedo.Attach(m_input.geometry_pass.albedo);
        m_program.ps.srv.gMaterial.Attach(m_input.geometry_pass.material);
        m_program.ps.srv.gSSAO.Attach(m_input.ssao_pass.srv_blur);
        m_program.ps.srv.irradianceMap.Attach(m_input.irradiance_pass.irradince);
        m_program.ps.srv.prefilterMap.Attach(m_input.irradiance_pass.prefilter);
        m_program.ps.srv.brdfLUT.Attach(m_input.irradiance_pass.brdf);

        m_context.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
    }
}

void LightPass::OnResize(int width, int height)
{
    m_width = width;
    m_height = height;
    CreateSizeDependentResources();
}

void LightPass::CreateSizeDependentResources()
{
    output.rtv = m_context.CreateTexture(BindFlag::kRtv | BindFlag::kSrv, gli::format::FORMAT_RGBA32_SFLOAT_PACK32, 1, m_width, m_height, 1);
    m_depth_stencil_view = m_context.CreateTexture(BindFlag::kDsv, gli::format::FORMAT_D24_UNORM_S8_UINT_PACK32, 1, m_width, m_height, 1);
}

void LightPass::OnModifySettings(const Settings& settings)
{
    Settings prev = m_settings;
    m_settings = settings;
    if (prev.msaa_count != m_settings.msaa_count)
    {
        m_program.ps.define["SAMPLE_COUNT"] = std::to_string(m_settings.msaa_count);
        m_program.ps.UpdateShader();
        m_program.LinkProgram();
    }
}
