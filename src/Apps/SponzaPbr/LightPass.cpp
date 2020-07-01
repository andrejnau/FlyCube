#include "LightPass.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

LightPass::LightPass(Device& device, const Input& input, int width, int height)
    : m_device(device)
    , m_input(input)
    , m_width(width)
    , m_height(height)
    , m_program(device, std::bind(&LightPass::SetDefines, this, std::placeholders::_1))
{
    CreateSizeDependentResources();
    m_sampler = m_device.CreateSampler({
        SamplerFilter::kAnisotropic,
        SamplerTextureAddressMode::kWrap,
        SamplerComparisonFunc::kNever });

    m_sampler_brdf = m_device.CreateSampler({
        SamplerFilter::kMinMagMipLinear,
        SamplerTextureAddressMode::kClamp,
        SamplerComparisonFunc::kNever });

    m_compare_sampler = m_device.CreateSampler({
        SamplerFilter::kComparisonMinMagMipLinear,
        SamplerTextureAddressMode::kClamp,
        SamplerComparisonFunc::kLess });
}

void LightPass::SetDefines(ProgramHolder<LightPassPS, LightPassVS>& program)
{
    if (m_settings.Get<uint32_t>("sample_count") != 1)
        program.ps.desc.define["SAMPLE_COUNT"] = std::to_string(m_settings.Get<uint32_t>("sample_count"));
}

void LightPass::OnUpdate()
{
    glm::vec3 camera_position = m_input.camera.GetCameraPos();

    m_program.ps.cbuffer.Light.viewPos = camera_position;
    m_program.ps.cbuffer.Settings.use_ssao = m_settings.Get<bool>("use_ssao") || m_settings.Get<bool>("use_rtao");
    m_program.ps.cbuffer.Settings.use_ao = m_settings.Get<bool>("use_ao");
    m_program.ps.cbuffer.Settings.use_IBL_diffuse = m_settings.Get<bool>("use_IBL_diffuse");
    m_program.ps.cbuffer.Settings.use_IBL_specular = m_settings.Get<bool>("use_IBL_specular");
    m_program.ps.cbuffer.Settings.only_ambient = m_settings.Get<bool>("only_ambient");
    m_program.ps.cbuffer.Settings.ambient_power = m_settings.Get<float>("ambient_power");
    m_program.ps.cbuffer.Settings.light_power = m_settings.Get<float>("light_power");
    m_program.ps.cbuffer.Settings.use_spec_ao_by_ndotv_roughness = m_settings.Get<bool>("use_spec_ao_by_ndotv_roughness");
    m_program.ps.cbuffer.Settings.show_only_position = m_settings.Get<bool>("show_only_position");
    m_program.ps.cbuffer.Settings.show_only_albedo = m_settings.Get<bool>("show_only_albedo");
    m_program.ps.cbuffer.Settings.show_only_normal = m_settings.Get<bool>("show_only_normal");
    m_program.ps.cbuffer.Settings.show_only_roughness = m_settings.Get<bool>("show_only_roughness");
    m_program.ps.cbuffer.Settings.show_only_metalness = m_settings.Get<bool>("show_only_metalness");
    m_program.ps.cbuffer.Settings.show_only_ao = m_settings.Get<bool>("show_only_ao");
    m_program.ps.cbuffer.Settings.use_f0_with_roughness = m_settings.Get<bool>("use_f0_with_roughness");

    m_program.ps.cbuffer.ShadowParams.s_near = m_settings.Get<float>("s_near");
    m_program.ps.cbuffer.ShadowParams.s_far = m_settings.Get<float>("s_far");
    m_program.ps.cbuffer.ShadowParams.s_size = m_settings.Get<float>("s_size");
    m_program.ps.cbuffer.ShadowParams.use_shadow = m_settings.Get<bool>("use_shadow");
    m_program.ps.cbuffer.ShadowParams.shadow_light_pos = m_input.light_pos;

    for (size_t i = 0; i < std::size(m_program.ps.cbuffer.Light.light_pos); ++i)
    {
        m_program.ps.cbuffer.Light.light_pos[i] = glm::vec4(0);
        m_program.ps.cbuffer.Light.light_color[i] = glm::vec4(0);
    }

    m_program.ps.cbuffer.Light.light_count = 0;
    if (m_settings.Get<bool>("light_in_camera"))
    {
        m_program.ps.cbuffer.Light.light_pos[m_program.ps.cbuffer.Light.light_count] = glm::vec4(camera_position, 0);
        m_program.ps.cbuffer.Light.light_color[m_program.ps.cbuffer.Light.light_count] = glm::vec4(1, 1, 1, 0.0);
        ++m_program.ps.cbuffer.Light.light_count;
    }
    if (m_settings.Get<bool>("additional_lights"))
    {
        for (int x = -13; x <= 13; ++x)
        {
            int q = 1;
            for (int z = -1; z <= 1; ++z)
            {
                if (m_program.ps.cbuffer.Light.light_count < std::size(m_program.ps.cbuffer.Light.light_pos))
                {
                    m_program.ps.cbuffer.Light.light_pos[m_program.ps.cbuffer.Light.light_count] = glm::vec4(x, 1.5, z - 0.33, 0);
                    float color = 0.0;
                    if (m_settings.Get<bool>("use_white_ligth"))
                        color = 1;
                    m_program.ps.cbuffer.Light.light_color[m_program.ps.cbuffer.Light.light_count] = glm::vec4(q == 1 ? 1 : color, q == 2 ? 1 : color, q == 3 ? 1 : color, 0.0);
                    ++m_program.ps.cbuffer.Light.light_count;
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

    glm::vec4 color = { 0.0f, 0.0f, 0.0f, 1.0f };
    command_list.Attach(m_program.ps.om.rtv0, output.rtv);
    command_list.ClearColor(m_program.ps.om.rtv0, color);
    command_list.Attach(m_program.ps.om.dsv, m_depth_stencil_view);
    command_list.ClearDepth(m_program.ps.om.dsv, 1.0f);

    m_input.model.ia.indices.Bind(command_list);
    m_input.model.ia.positions.BindToSlot(command_list, m_program.vs.ia.POSITION);
    m_input.model.ia.texcoords.BindToSlot(command_list, m_program.vs.ia.TEXCOORD);

    for (auto& range : m_input.model.ia.ranges)
    {
        command_list.Attach(m_program.ps.srv.gNormal, m_input.geometry_pass.normal);
        command_list.Attach(m_program.ps.srv.gAlbedo, m_input.geometry_pass.albedo);
        command_list.Attach(m_program.ps.srv.gMaterial, m_input.geometry_pass.material);
        if (m_settings.Get<bool>("use_rtao") && m_input.ray_tracing_ao)
            command_list.Attach(m_program.ps.srv.gSSAO, *m_input.ray_tracing_ao);
        else if (m_settings.Get<bool>("use_ssao"))
            command_list.Attach(m_program.ps.srv.gSSAO, m_input.ssao_pass.ao);
        command_list.Attach(m_program.ps.srv.irradianceMap, m_input.irradince);
        command_list.Attach(m_program.ps.srv.prefilterMap, m_input.prefilter);
        command_list.Attach(m_program.ps.srv.brdfLUT, m_input.brdf);
        if (m_settings.Get<bool>("use_shadow"))
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
    output.rtv = m_device.CreateTexture(BindFlag::kRenderTarget | BindFlag::kShaderResource, gli::format::FORMAT_RGBA32_SFLOAT_PACK32, 1, m_width, m_height, 1);
    m_depth_stencil_view = m_device.CreateTexture(BindFlag::kDepthStencil, gli::format::FORMAT_D32_SFLOAT_PACK32, 1, m_width, m_height, 1);
}

void LightPass::OnModifySponzaSettings(const SponzaSettings& settings)
{
    SponzaSettings prev = m_settings;
    m_settings = settings;
    if (prev.Get<uint32_t>("sample_count") != m_settings.Get<uint32_t>("sample_count"))
    {
        m_program.ps.desc.define["SAMPLE_COUNT"] = std::to_string(m_settings.Get<uint32_t>("sample_count"));
        m_program.UpdateProgram();
    }
}
