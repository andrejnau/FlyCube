#include "Scene.h"
#include <Utilities/DXUtility.h>
#include <Utilities/FileUtility.h>
#include <Utilities/State.h>
#include <D3Dcompiler.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <glm/gtx/transform.hpp>
#include <Context/ContextSelector.h>

Scene::Scene(ApiType type, GLFWwindow* window, int width, int height)
    : m_width(width)
    , m_height(height)
    , m_context_ptr(CreateContext(type, window, m_width, m_height))
    , m_context(*m_context_ptr)
    , m_model_square(m_context, "model/square.obj")
    , m_model_cube(m_context, "model/cube.obj", ~aiProcess_FlipWindingOrder)
    , m_geometry_pass(m_context, { m_scene_list, m_camera }, width, height)
    , m_ssao_pass(m_context, { m_geometry_pass.output, m_model_square, m_camera }, width, height)
    , m_irradiance_conversion(m_context, { m_model_cube, m_model_square, m_equirectangular_environment }, width, height)
    , m_light_pass(m_context, { m_geometry_pass.output, m_ssao_pass.output, m_model_square, m_camera, m_irradiance_conversion.output }, width, height)
    , m_background_pass(m_context, { m_model_cube, m_camera, m_irradiance_conversion.output.environment, m_light_pass.output.rtv, m_geometry_pass.output.dsv }, width, height)
    , m_compute_luminance(m_context, { m_light_pass.output.rtv, m_model_square, m_render_target_view, m_depth_stencil_view }, width, height)
    , m_imgui_pass(m_context, { m_render_target_view, *this }, width, height)
{
    m_scene_list.emplace_back(m_context, "model/sponza_pbr/sponza.obj");
    m_scene_list.back().matrix = glm::scale(glm::vec3(0.01f));
    m_scene_list.emplace_back(m_context, "model/export3dcoat/export3dcoat.obj");
    m_scene_list.back().matrix = glm::scale(glm::vec3(0.07f)) * glm::translate(glm::vec3(0.0f, 75.0f, 0.0f)) * glm::rotate(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    std::string hdr_tests[] =
    {
        "gold",
        "grass",
        "plastic",
        "rusted_iron",
        "wall",
    };

    float x = 300;
    for (const auto& test : hdr_tests)
    {
        m_scene_list.emplace_back(m_context, "model/pbr_test/" + test + "/sphere.obj");
        m_scene_list.back().matrix = glm::scale(glm::vec3(0.01f)) *glm::translate(glm::vec3(x, 1500, 0.0f));
        x += 50;
    }

    CreateRT();

    m_equirectangular_environment = CreateTexture(m_context, GetAssetFullPath("model/newport_loft.dds"));
}

Scene::~Scene()
{
    m_context.OnDestroy();
}

IScene::Ptr Scene::Create(ApiType api_type, GLFWwindow* window, int width, int height)
{
    return std::make_unique<Scene>(api_type, window, width, height);
}

void Scene::OnUpdate()
{
    UpdateCameraMovement();

    m_imgui_pass.OnUpdate();

    m_geometry_pass.OnUpdate();
    m_ssao_pass.OnUpdate();
    m_irradiance_conversion.OnUpdate();
    m_light_pass.OnUpdate();
    m_background_pass.OnUpdate();
    m_compute_luminance.OnUpdate();
}

void Scene::OnRender()
{
    m_render_target_view = m_context.GetBackBuffer();
    m_camera.SetViewport(m_width, m_height);

    m_context.BeginEvent("Geometry Pass");
    m_geometry_pass.OnRender();
    m_context.EndEvent();

    m_context.BeginEvent("SSAO Pass");
    m_ssao_pass.OnRender();
    m_context.EndEvent();

    m_context.BeginEvent("Irradiance Conversion Pass");
    m_irradiance_conversion.OnRender();
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

    m_context.Present(m_render_target_view);
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

    m_geometry_pass.OnResize(width, height);
    m_ssao_pass.OnResize(width, height);
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

    if (key == GLFW_KEY_N && action == GLFW_PRESS)
        CurState::Instance().disable_norm ^= true;

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
        CurState::Instance().pause ^= true;

    if (key == GLFW_KEY_J && action == GLFW_PRESS)
        CurState::Instance().no_shadow_discard ^= true;
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

void Scene::OnModifySettings(const Settings& settings)
{
    m_geometry_pass.OnModifySettings(settings);
    m_light_pass.OnModifySettings(settings);
    m_compute_luminance.OnModifySettings(settings);
    m_ssao_pass.OnModifySettings(settings);
    m_irradiance_conversion.OnModifySettings(settings);
    m_background_pass.OnModifySettings(settings);
}

void Scene::CreateRT()
{
    m_depth_stencil_view = m_context.CreateTexture((BindFlag)(BindFlag::kDsv), gli::format::FORMAT_D24_UNORM_S8_UINT_PACK32, 1, m_width, m_height, 1);
}
