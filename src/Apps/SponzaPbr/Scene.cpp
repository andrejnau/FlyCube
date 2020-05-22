#include "Scene.h"
#include <Utilities/FileUtility.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <glm/gtx/transform.hpp>

Scene::Scene(Context& context, int width, int height)
    : m_context(context)
    , m_width(width)
    , m_height(height)
    , m_model_square(m_context, "model/square.obj")
    , m_model_cube(m_context, "model/cube.obj", ~aiProcess_FlipWindingOrder)
    , m_skinning_pass(m_context, { m_scene_list }, width, height)
    , m_geometry_pass(m_context, { m_scene_list, m_camera }, width, height)
    , m_shadow_pass(m_context, { m_scene_list, m_camera, m_light_pos }, width, height)
    , m_ssao_pass(m_context, { m_geometry_pass.output, m_model_square, m_camera }, width, height)
    , m_brdf(m_context, { m_model_square }, width, height)
    , m_equirectangular2cubemap(m_context, { m_model_cube, m_equirectangular_environment }, width, height)
    , m_ibl_compute(m_context, { m_shadow_pass.output, m_scene_list, m_camera, m_light_pos, m_model_cube, m_equirectangular2cubemap.output.environment }, width, height)
    , m_light_pass(m_context, { m_geometry_pass.output, m_shadow_pass.output, m_ssao_pass.output, m_rtao, m_model_square, m_camera, m_light_pos, m_irradince, m_prefilter, m_brdf.output.brdf }, width, height)
    , m_background_pass(m_context, { m_model_cube, m_camera, m_equirectangular2cubemap.output.environment, m_light_pass.output.rtv, m_geometry_pass.output.dsv }, width, height)
    , m_compute_luminance(m_context, { m_light_pass.output.rtv, m_model_square, m_render_target_view, m_depth_stencil_view }, width, height)
    , m_imgui_pass(m_context, { m_render_target_view, *this }, width, height)
{
#if !defined(_DEBUG) && 1
    m_scene_list.emplace_back(m_context, "model/sponza_pbr/sponza.obj");
    m_scene_list.back().matrix = glm::scale(glm::vec3(0.01f));
#endif

#if 1
    m_scene_list.emplace_back(m_context, "model/export3dcoat/export3dcoat.obj");
    m_scene_list.back().matrix = glm::scale(glm::vec3(0.07f)) * glm::translate(glm::vec3(0.0f, 35.0f, 0.0f)) * glm::rotate(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_scene_list.back().ibl_request = true;
#endif

#if 0
    m_scene_list.emplace_back(m_context, "model/Mannequin_Animation/source/Mannequin_Animation.FBX");
    m_scene_list.back().matrix = glm::scale(glm::vec3(0.07f)) * glm::translate(glm::vec3(75.0f, 0.0f, 0.0f)) * glm::rotate(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
#endif

#if 0
    std::pair<std::string, bool> hdr_tests[] =
    {
        { "gold",        false },
        { "grass",       false },
        { "plastic",     false },
        { "rusted_iron", false },
        { "wall",        false },
    };

    float x = 300;
    for (const auto& test : hdr_tests)
    {
        m_scene_list.emplace_back(m_context, "model/pbr_test/" + test.first + "/sphere.obj");
        m_scene_list.back().matrix = glm::scale(glm::vec3(0.01f)) * glm::translate(glm::vec3(x, 500, 0.0f));
        m_scene_list.back().ibl_request = test.second;
        if (!test.second)
            m_scene_list.back().ibl_source = 0;
        x += 50;
    }
#endif

#if 0
    m_scene_list.emplace_back(m_context, "local_model/SunTemple_v3/SunTemple/SunTemple.fbx");
    m_scene_list.back().matrix = glm::scale(glm::vec3(0.01));
#endif

    for (auto& model : m_scene_list)
    {
        if (model.ibl_request)
            ++m_ibl_count;
    }

    CreateRT();

    m_equirectangular_environment = CreateTexture(m_context, GetAssetFullPath("model/newport_loft.dds"));

    size_t layer = 0;
    {
        IrradianceConversion::Target irradince{ m_irradince, m_depth_stencil_view_irradince, layer, m_irradince_texture_size };
        IrradianceConversion::Target prefilter{ m_prefilter, m_depth_stencil_view_prefilter, layer, m_prefilter_texture_size };
        m_irradiance_conversion.emplace_back(new IrradianceConversion(m_context, { m_model_cube, m_equirectangular2cubemap.output.environment, irradince, prefilter }, width, height));
    }

    for (auto& model : m_scene_list)
    {
        if (!model.ibl_request)
            continue;

        model.ibl_source = ++layer;
        IrradianceConversion::Target irradince{ m_irradince, m_depth_stencil_view_irradince, model.ibl_source, m_irradince_texture_size };
        IrradianceConversion::Target prefilter{ m_prefilter, m_depth_stencil_view_prefilter, model.ibl_source, m_prefilter_texture_size };
        m_irradiance_conversion.emplace_back(new IrradianceConversion(m_context, { m_model_cube, model.ibl_rtv, irradince, prefilter }, width, height));
    }

#ifdef RAYTRACING_SUPPORT
    if (m_context.IsDxrSupported())
    {
        m_ray_tracing_ao_pass.reset(new RayTracingAOPass(m_context, { m_geometry_pass.output, m_scene_list, m_model_square, m_camera }, width, height));
        m_rtao = &m_ray_tracing_ao_pass->output.ao;
    }
#endif
}

Scene::~Scene()
{
    m_context.OnDestroy();
}

IScene::Ptr Scene::Create(Context& context, int width, int height)
{
    return std::make_unique<Scene>(context, width, height);
}

void Scene::OnUpdate()
{
    UpdateCameraMovement();

    static float angle = 0.0f;
    static std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
    std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
    int64_t elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    start = end;

    if (m_settings.dynamic_sun_position)
        angle += elapsed / 2e6f;

    float light_r = 2.5;
    m_light_pos = glm::vec3(light_r * cos(angle), 25.0f, light_r * sin(angle));

    m_imgui_pass.OnUpdate();

    m_skinning_pass.OnUpdate();
    m_geometry_pass.OnUpdate();
    m_shadow_pass.OnUpdate();
    m_ssao_pass.OnUpdate();
#ifdef RAYTRACING_SUPPORT
    if (m_ray_tracing_ao_pass)
        m_ray_tracing_ao_pass->OnUpdate();
#endif
    m_brdf.OnUpdate();
    m_equirectangular2cubemap.OnUpdate();
    m_ibl_compute.OnUpdate();
    for (auto& x : m_irradiance_conversion)
    {
        x->OnUpdate();
    }
    m_light_pass.OnUpdate();
    m_background_pass.OnUpdate();
    m_compute_luminance.OnUpdate();
}

void Scene::OnRender()
{
    m_render_target_view = m_context.GetBackBuffer();
    m_camera.SetViewport(m_width, m_height);

    m_context.BeginEvent("Skinning Pass");
    m_skinning_pass.OnRender();
    m_context.EndEvent();

    m_context.BeginEvent("Geometry Pass");
    m_geometry_pass.OnRender();
    m_context.EndEvent();

    m_context.BeginEvent("Shadow Pass");
    m_shadow_pass.OnRender();
    m_context.EndEvent();

    m_context.BeginEvent("SSAO Pass");
    m_ssao_pass.OnRender();
    m_context.EndEvent();

#ifdef RAYTRACING_SUPPORT
    if (m_ray_tracing_ao_pass)
    {
        m_context.BeginEvent("DXR AO Pass");
        m_ray_tracing_ao_pass->OnRender();
        m_context.EndEvent();
    }
#endif

    m_context.BeginEvent("brdf Pass");
    m_brdf.OnRender();
    m_context.EndEvent();

    m_context.BeginEvent("equirectangular to cubemap Pass");
    m_equirectangular2cubemap.OnRender();
    m_context.EndEvent();

    m_context.BeginEvent("IBLCompute");
    m_ibl_compute.OnRender();
    m_context.EndEvent();

    m_context.BeginEvent("Irradiance Conversion Pass");
    for (auto& x : m_irradiance_conversion)
    {
        x->OnRender();
    }
    m_context.EndEvent();

    m_context.BeginEvent("Light Pass");
    m_light_pass.OnRender();
    m_context.EndEvent();

    m_context.BeginEvent("Background Pass");
    m_background_pass.OnRender();
    m_context.EndEvent();

    m_context.BeginEvent("HDR Pass");
    m_compute_luminance.OnRender();
    m_context.EndEvent();

    if (glfwGetInputMode(m_context.GetWindow(), GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
    {
        m_context.BeginEvent("ImGui Pass");
        m_imgui_pass.OnRender();
        m_context.EndEvent();
    }

    m_context.Present();
}

void Scene::OnResize(int width, int height)
{
    if (width == m_width && height == m_height)
        return;

    m_width = width;
    m_height = height;

    m_render_target_view.reset();

    m_context.OnResize(width, height);

    CreateRT();

    m_skinning_pass.OnResize(width, height);
    m_geometry_pass.OnResize(width, height);
    m_shadow_pass.OnResize(width, height);
    m_ssao_pass.OnResize(width, height);
    m_ibl_compute.OnResize(width, height);
    m_light_pass.OnResize(width, height);
    m_compute_luminance.OnResize(width, height);
    m_imgui_pass.OnResize(width, height);
}

void Scene::OnKey(int key, int action)
{
    m_imgui_pass.OnKey(key, action);
    if (glfwGetInputMode(m_context.GetWindow(), GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
        return;

    if (action == GLFW_PRESS)
        m_keys[key] = true;
    else if (action == GLFW_RELEASE)
        m_keys[key] = false;
}

void Scene::OnMouse(bool first_event, double xpos, double ypos)
{
    if (glfwGetInputMode(m_context.GetWindow(), GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
    {
        m_imgui_pass.OnMouse(first_event, xpos, ypos);
        return;
    }

    if (first_event)
    {
        m_last_x = xpos;
        m_last_y = ypos;
    }

    double xoffset = xpos - m_last_x;
    double yoffset = m_last_y - ypos;

    m_last_x = xpos;
    m_last_y = ypos;

    m_camera.ProcessMouseMovement((float)xoffset, (float)yoffset);
}

void Scene::OnMouseButton(int button, int action)
{
    if (glfwGetInputMode(m_context.GetWindow(), GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
    {
        m_imgui_pass.OnMouseButton(button, action);
    }
}

void Scene::OnScroll(double xoffset, double yoffset)
{
    if (glfwGetInputMode(m_context.GetWindow(), GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
    {
        m_imgui_pass.OnScroll(xoffset, yoffset);
    }
}

void Scene::OnInputChar(unsigned int ch)
{
    if (glfwGetInputMode(m_context.GetWindow(), GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
    {
        m_imgui_pass.OnInputChar(ch);
    }
}

void Scene::OnModifySponzaSettings(const SponzaSettings& settings)
{
    m_settings = settings;
    m_skinning_pass.OnModifySponzaSettings(settings);
    m_geometry_pass.OnModifySponzaSettings(settings);
    m_shadow_pass.OnModifySponzaSettings(settings);
    m_ibl_compute.OnModifySponzaSettings(settings);
    m_light_pass.OnModifySponzaSettings(settings);
    m_compute_luminance.OnModifySponzaSettings(settings);
    m_ssao_pass.OnModifySponzaSettings(settings);
#ifdef RAYTRACING_SUPPORT
    if (m_ray_tracing_ao_pass)
        m_ray_tracing_ao_pass->OnModifySponzaSettings(settings);
#endif
    m_brdf.OnModifySponzaSettings(settings);
    m_equirectangular2cubemap.OnModifySponzaSettings(settings);
    for (auto& x : m_irradiance_conversion)
    {
        x->OnModifySponzaSettings(settings);
    }
    m_background_pass.OnModifySponzaSettings(settings);
}

void Scene::CreateRT()
{
    m_depth_stencil_view = m_context.CreateTexture(BindFlag::kDsv, gli::format::FORMAT_D24_UNORM_S8_UINT_PACK32, 1, m_width, m_height, 1);

    m_irradince = m_context.CreateTexture(BindFlag::kRtv | BindFlag::kSrv, gli::format::FORMAT_RGBA32_SFLOAT_PACK32, 1, 
        m_irradince_texture_size, m_irradince_texture_size, 6 * m_ibl_count);
    m_prefilter = m_context.CreateTexture(BindFlag::kRtv | BindFlag::kSrv, gli::format::FORMAT_RGBA32_SFLOAT_PACK32, 1, 
        m_prefilter_texture_size, m_prefilter_texture_size, 6 * m_ibl_count, log2(m_prefilter_texture_size));

    m_depth_stencil_view_irradince = m_context.CreateTexture(BindFlag::kDsv, gli::format::FORMAT_D32_SFLOAT_PACK32, 1, m_irradince_texture_size, m_irradince_texture_size, 6 * m_ibl_count);
    m_depth_stencil_view_prefilter = m_context.CreateTexture(BindFlag::kDsv, gli::format::FORMAT_D32_SFLOAT_PACK32, 1, m_prefilter_texture_size, m_prefilter_texture_size, 6 * m_ibl_count, log2(m_prefilter_texture_size));
}
