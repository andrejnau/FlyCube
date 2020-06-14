#include "Scene.h"
#include <Utilities/FileUtility.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <glm/gtx/transform.hpp>

Scene::Scene(Context& context, int width, int height)
    : m_context(context)
    , m_device(*context.GetDevice())
    , m_width(width)
    , m_height(height)
    , m_upload_command_list(context.CreateCommandList(true))
    , m_model_square(m_device, *m_upload_command_list, "model/square.obj")
    , m_model_cube(m_device, *m_upload_command_list, "model/cube.obj", ~aiProcess_FlipWindingOrder)
    , m_skinning_pass(m_device, { m_scene_list })
    , m_geometry_pass(m_device, { m_scene_list, m_camera }, width, height)
    , m_shadow_pass(m_device, { m_scene_list, m_camera, m_light_pos })
    , m_ssao_pass(m_device, *m_upload_command_list, { m_geometry_pass.output, m_model_square, m_camera }, width, height)
    , m_brdf(m_device, { m_model_square })
    , m_equirectangular2cubemap(m_device, { m_model_cube, m_equirectangular_environment })
    , m_ibl_compute(m_device, { m_shadow_pass.output, m_scene_list, m_camera, m_light_pos, m_model_cube, m_equirectangular2cubemap.output.environment })
    , m_background_pass(m_device, { m_model_cube, m_camera, m_equirectangular2cubemap.output.environment, m_geometry_pass.output.albedo, m_geometry_pass.output.dsv }, width, height)
    , m_light_pass(m_device, { m_geometry_pass.output, m_shadow_pass.output, m_ssao_pass.output, m_rtao, m_model_square, m_camera, m_light_pos, m_irradince, m_prefilter, m_brdf.output.brdf }, width, height)
    , m_compute_luminance(m_device, { m_light_pass.output.rtv, m_model_square, m_render_target_view, m_depth_stencil_view }, width, height)
    , m_imgui_pass(m_device, *m_upload_command_list, { m_render_target_view, *this }, width, height, context.GetWindow())
{
#if !defined(_DEBUG) && 1
    m_scene_list.emplace_back(m_device, *m_upload_command_list, "model/sponza_pbr/sponza.obj");
    m_scene_list.back().matrix = glm::scale(glm::vec3(0.01f));
#endif

#if 1
    m_scene_list.emplace_back(m_device, *m_upload_command_list, "model/export3dcoat/export3dcoat.obj");
    m_scene_list.back().matrix = glm::scale(glm::vec3(0.07f)) * glm::translate(glm::vec3(0.0f, 35.0f, 0.0f)) * glm::rotate(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_scene_list.back().ibl_request = true;
#endif

#if 0
    m_scene_list.emplace_back(m_device, *m_upload_command_list, "model/Mannequin_Animation/source/Mannequin_Animation.FBX");
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
        m_scene_list.emplace_back(m_device, *m_upload_command_list, "model/pbr_test/" + test.first + "/sphere.obj");
        m_scene_list.back().matrix = glm::scale(glm::vec3(0.01f)) * glm::translate(glm::vec3(x, 500, 0.0f));
        m_scene_list.back().ibl_request = test.second;
        if (!test.second)
            m_scene_list.back().ibl_source = 0;
        x += 50;
    }
#endif

    for (auto& model : m_scene_list)
    {
        if (model.ibl_request)
            ++m_ibl_count;
    }

    CreateRT();

    m_equirectangular_environment = CreateTexture(m_device, *m_upload_command_list, GetAssetFullPath("model/newport_loft.dds"));

    size_t layer = 0;
    {
        IrradianceConversion::Target irradince{ m_irradince, m_depth_stencil_view_irradince, layer, m_irradince_texture_size };
        IrradianceConversion::Target prefilter{ m_prefilter, m_depth_stencil_view_prefilter, layer, m_prefilter_texture_size };
        m_irradiance_conversion.emplace_back(new IrradianceConversion(m_device, { m_model_cube, m_equirectangular2cubemap.output.environment, irradince, prefilter }));
    }

    for (auto& model : m_scene_list)
    {
        if (!model.ibl_request)
            continue;

        model.ibl_source = ++layer;
        IrradianceConversion::Target irradince{ m_irradince, m_depth_stencil_view_irradince, model.ibl_source, m_irradince_texture_size };
        IrradianceConversion::Target prefilter{ m_prefilter, m_depth_stencil_view_prefilter, model.ibl_source, m_prefilter_texture_size };
        m_irradiance_conversion.emplace_back(new IrradianceConversion(m_device, { m_model_cube, model.ibl_rtv, irradince, prefilter }));
    }

#ifdef RAYTRACING_SUPPORT
    if (m_device.IsDxrSupported())
    {
        m_ray_tracing_ao_pass.reset(new RayTracingAOPass(m_device, *m_upload_command_list, { m_geometry_pass.output, m_scene_list, m_model_square, m_camera }, width, height));
        m_rtao = &m_ray_tracing_ao_pass->output.ao;
    }
#endif

    m_passes.push_back({ "Skinning Pass", m_skinning_pass });
    m_passes.push_back({ "Geometry Pass", m_geometry_pass });
    m_passes.push_back({ "Shadow Pass", m_shadow_pass });
    m_passes.push_back({ "SSAO Pass", m_ssao_pass });
#ifdef RAYTRACING_SUPPORT
    if (m_ray_tracing_ao_pass)
        m_passes.push_back({ "DXR AO Pass", *m_ray_tracing_ao_pass });
#endif

    m_passes.push_back({ "brdf Pass", m_brdf });
    m_passes.push_back({ "equirectangular to cubemap Pass", m_equirectangular2cubemap });
    m_passes.push_back({ "IBLCompute", m_ibl_compute });
    for (auto& x : m_irradiance_conversion)
        m_passes.push_back({ "Irradiance Conversion Pass", *x });
    m_passes.push_back({ "Background Pass", m_background_pass });
    m_passes.push_back({ "Light Pass", m_light_pass });
    m_passes.push_back({ "HDR Pass", m_compute_luminance });
    m_passes.push_back({ "ImGui Pass", m_imgui_pass });

    m_camera.SetCameraPos(glm::vec3(-3.0, 2.75, 0.0));
    m_camera.SetCameraYaw(-178.0f);
    m_camera.SetCameraYaw(-1.75f);
    m_camera.SetViewport(m_width, m_height);

    for (uint32_t i = 0; i < Context::FrameCount * m_passes.size(); ++i)
        m_command_lists.emplace_back(m_context.CreateCommandList());

    m_upload_command_list->Close();
    m_context.ExecuteCommandLists({ m_upload_command_list });
}

void Scene::RenderFrame()
{
    UpdateCameraMovement();

    static float angle = 0.0f;
    static std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
    std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
    int64_t elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    start = end;

    if (m_settings.Get<bool>("dynamic_sun_position"))
        angle += elapsed / 2e6f;

    float light_r = 2.5;
    m_light_pos = glm::vec3(light_r * cos(angle), 25.0f, light_r * sin(angle));

    for (auto& desc : m_passes)
    {
        desc.pass.get().OnUpdate();
    }

    m_render_target_view = m_context.GetBackBuffer(m_context.GetFrameIndex());

    std::vector<std::shared_ptr<CommandListBox>> command_lists;

    for (auto& desc : m_passes)
    {
        decltype(auto) command_list = m_command_lists[m_command_list_index];
        m_command_list_index = (m_command_list_index + 1) % m_command_lists.size();
        command_list->Open();
        command_list->BeginEvent(desc.name);
        desc.pass.get().OnRender(*command_list);
        command_list->EndEvent();
        command_list->Close();

        command_lists.emplace_back(command_list);
    }

    m_context.ExecuteCommandLists(command_lists);
    m_context.Present();
}

void Scene::OnResize(int width, int height)
{
    if (width == m_width && height == m_height)
        return;

    m_width = width;
    m_height = height;

    m_render_target_view.reset();
    m_context.WaitIdle();

    for (auto& cmd : m_command_lists)
        cmd = m_context.CreateCommandList();

    m_context.Resize(m_width, m_height);
    m_camera.SetViewport(m_width, m_height);

    CreateRT();

    for (auto& desc : m_passes)
    {
        desc.pass.get().OnResize(width, height);
    }
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
    for (auto& desc : m_passes)
    {
        desc.pass.get().OnModifySponzaSettings(m_settings);
    }
}

void Scene::CreateRT()
{
    m_depth_stencil_view = m_device.CreateTexture(BindFlag::kDepthStencil, gli::format::FORMAT_D32_SFLOAT_PACK32, 1, m_width, m_height, 1);

    if (m_irradince)
        return;

    m_irradince = m_device.CreateTexture(BindFlag::kRenderTarget | BindFlag::kShaderResource, gli::format::FORMAT_RGBA32_SFLOAT_PACK32, 1, 
        m_irradince_texture_size, m_irradince_texture_size, 6 * m_ibl_count);
    m_prefilter = m_device.CreateTexture(BindFlag::kRenderTarget | BindFlag::kShaderResource, gli::format::FORMAT_RGBA32_SFLOAT_PACK32, 1, 
        m_prefilter_texture_size, m_prefilter_texture_size, 6 * m_ibl_count, log2(m_prefilter_texture_size));

    m_depth_stencil_view_irradince = m_device.CreateTexture(BindFlag::kDepthStencil, gli::format::FORMAT_D32_SFLOAT_PACK32, 1, m_irradince_texture_size, m_irradince_texture_size, 6 * m_ibl_count);
    m_depth_stencil_view_prefilter = m_device.CreateTexture(BindFlag::kDepthStencil, gli::format::FORMAT_D32_SFLOAT_PACK32, 1, m_prefilter_texture_size, m_prefilter_texture_size, 6 * m_ibl_count, log2(m_prefilter_texture_size));
}

void Scene::UpdateCameraMovement()
{
    float currentFrame = static_cast<float>(glfwGetTime());
    m_delta_time = currentFrame - m_last_frame;
    m_last_frame = currentFrame;

    if (m_keys[GLFW_KEY_W])
        m_camera.ProcessKeyboard(CameraMovement::kForward, m_delta_time);
    if (m_keys[GLFW_KEY_S])
        m_camera.ProcessKeyboard(CameraMovement::kBackward, m_delta_time);
    if (m_keys[GLFW_KEY_A])
        m_camera.ProcessKeyboard(CameraMovement::kLeft, m_delta_time);
    if (m_keys[GLFW_KEY_D])
        m_camera.ProcessKeyboard(CameraMovement::kRight, m_delta_time);
    if (m_keys[GLFW_KEY_Q])
        m_camera.ProcessKeyboard(CameraMovement::kDown, m_delta_time);
    if (m_keys[GLFW_KEY_E])
        m_camera.ProcessKeyboard(CameraMovement::kUp, m_delta_time);
}
