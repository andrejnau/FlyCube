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
    CreateSizeDependentResources();
    m_sampler = m_context.CreateSampler({
        SamplerFilter::kAnisotropic,
        SamplerTextureAddressMode::kWrap,
        SamplerComparisonFunc::kNever });

    m_sampler_brdf = m_context.CreateSampler({
        SamplerFilter::kMinMagMipLinear,
        SamplerTextureAddressMode::kClamp,
        SamplerComparisonFunc::kNever });

    m_compare_sampler = m_context.CreateSampler({
        SamplerFilter::kComparisonMinMagMipLinear,
        SamplerTextureAddressMode::kClamp,
        SamplerComparisonFunc::kLess });
}

void LightPass::SetDefines(ProgramHolder<LightPassPS, LightPassVS>& program)
{
    if (m_settings.msaa_count != 1)
        program.ps.desc.define["SAMPLE_COUNT"] = std::to_string(m_settings.msaa_count);
}

void LightPass::OnUpdate()
{
    glm::vec3 camera_position = m_input.camera.GetCameraPos();

    m_program.ps.cbuffer.Light.viewPos = camera_position;
    m_program.ps.cbuffer.Settings.use_ssao = m_settings.use_ssao || m_settings.use_rtao;
    m_program.ps.cbuffer.Settings.use_ao = m_settings.use_ao;
    m_program.ps.cbuffer.Settings.use_IBL_diffuse = m_settings.use_IBL_diffuse;
    m_program.ps.cbuffer.Settings.use_IBL_specular = m_settings.use_IBL_specular;
    m_program.ps.cbuffer.Settings.only_ambient = m_settings.only_ambient;
    m_program.ps.cbuffer.Settings.ambient_power = m_settings.ambient_power;
    m_program.ps.cbuffer.Settings.light_power = m_settings.light_power;
    m_program.ps.cbuffer.Settings.use_spec_ao_by_ndotv_roughness = m_settings.use_spec_ao_by_ndotv_roughness;
    m_program.ps.cbuffer.Settings.show_only_position = m_settings.show_only_position;
    m_program.ps.cbuffer.Settings.show_only_albedo = m_settings.show_only_albedo;
    m_program.ps.cbuffer.Settings.show_only_normal = m_settings.show_only_normal;
    m_program.ps.cbuffer.Settings.show_only_roughness = m_settings.show_only_roughness;
    m_program.ps.cbuffer.Settings.show_only_metalness = m_settings.show_only_metalness;
    m_program.ps.cbuffer.Settings.show_only_ao = m_settings.show_only_ao;
    m_program.ps.cbuffer.Settings.use_f0_with_roughness = m_settings.use_f0_with_roughness;

    m_program.ps.cbuffer.ShadowParams.s_near = m_settings.s_near;
    m_program.ps.cbuffer.ShadowParams.s_far = m_settings.s_far;
    m_program.ps.cbuffer.ShadowParams.s_size = m_settings.s_size;
    m_program.ps.cbuffer.ShadowParams.use_shadow = m_settings.use_shadow;
    m_program.ps.cbuffer.ShadowParams.shadow_light_pos = m_input.light_pos;

    for (size_t i = 0; i < std::size(m_program.ps.cbuffer.Light.light_pos); ++i)
    {
        m_program.ps.cbuffer.Light.light_pos[i] = glm::vec4(0);
        m_program.ps.cbuffer.Light.light_color[i] = glm::vec4(0);
    }

    if (m_settings.light_in_camera)
    {
        m_program.ps.cbuffer.Light.light_pos[0] = glm::vec4(camera_position, 0);
        m_program.ps.cbuffer.Light.light_color[0] = glm::vec4(1, 1, 1, 0.0);
    }
    if (m_settings.additional_lights)
    {
        int i = 0;
        if (m_settings.light_in_camera)
            ++i;
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
                    m_program.ps.cbuffer.Light.light_color[i] = glm::vec4(q == 1 ? 1 : color, q == 2 ? 1 : color, q == 3 ? 1 : color, 0.0);
                    ++i;
                    ++q;
                }
            }
        }
    }

    glm::mat4 projection, view, model;
    m_input.camera.GetMatrix(projection, view, model);
    m_program.ps.cbuffer.Light.inverted_mvp = glm::transpose(glm::inverse(projection * view));
}

void LightPass::OnRender(CommandListBox& command_list)
{
    command_list.SetViewport(m_width, m_height);

    command_list.UseProgram(m_program);
    command_list.Attach(m_program.ps.cbv.Light, m_program.ps.cbuffer.Light);
    command_list.Attach(m_program.ps.cbv.Settings, m_program.ps.cbuffer.Settings);
    command_list.Attach(m_program.ps.cbv.ShadowParams, m_program.ps.cbuffer.ShadowParams);

    command_list.Attach(m_program.ps.sampler.g_sampler, m_sampler);
    command_list.Attach(m_program.ps.sampler.brdf_sampler, m_sampler_brdf);
    command_list.Attach(m_program.ps.sampler.LightCubeShadowComparsionSampler, m_compare_sampler);

    std::array<float, 4> color = { 0.0f, 0.0f, 0.0f, 1.0f };
    command_list.Attach(m_program.ps.om.rtv0, output.rtv);
    command_list.ClearColor(m_program.ps.om.rtv0, color);
    command_list.Attach(m_program.ps.om.dsv, m_depth_stencil_view);
    command_list.ClearDepth(m_program.ps.om.dsv, 1.0f);

    m_input.model.ia.indices.Bind();
    m_input.model.ia.positions.BindToSlot(m_program.vs.ia.POSITION);
    m_input.model.ia.texcoords.BindToSlot(m_program.vs.ia.TEXCOORD);

    for (auto& range : m_input.model.ia.ranges)
    {
        command_list.Attach(m_program.ps.srv.gNormal, m_input.geometry_pass.normal);
        command_list.Attach(m_program.ps.srv.gAlbedo, m_input.geometry_pass.albedo);
        command_list.Attach(m_program.ps.srv.gMaterial, m_input.geometry_pass.material);
        if (m_settings.use_rtao && m_input.ray_tracing_ao)
            command_list.Attach(m_program.ps.srv.gSSAO, *m_input.ray_tracing_ao);
        else if (m_settings.use_ssao)
            command_list.Attach(m_program.ps.srv.gSSAO, m_input.ssao_pass.ao);
        command_list.Attach(m_program.ps.srv.irradianceMap, m_input.irradince);
        command_list.Attach(m_program.ps.srv.prefilterMap, m_input.prefilter);
        command_list.Attach(m_program.ps.srv.brdfLUT, m_input.brdf);
        if (m_settings.use_shadow)
            command_list.Attach(m_program.ps.srv.LightCubeShadowMap, m_input.shadow_pass.srv);

        command_list.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
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
    output.rtv = m_context.CreateTexture(BindFlag::kRenderTarget | BindFlag::kShaderResource, gli::format::FORMAT_RGBA32_SFLOAT_PACK32, 1, m_width, m_height, 1);
    m_depth_stencil_view = m_context.CreateTexture(BindFlag::kDepthStencil, gli::format::FORMAT_D24_UNORM_S8_UINT_PACK32, 1, m_width, m_height, 1);
}

void LightPass::OnModifySponzaSettings(const SponzaSettings& settings)
{
    SponzaSettings prev = m_settings;
    m_settings = settings;
    if (prev.msaa_count != m_settings.msaa_count)
    {
        m_program.ps.desc.define["SAMPLE_COUNT"] = std::to_string(m_settings.msaa_count);
        m_program.UpdateProgram();
    }
}
