#include "Scene/SceneBase.h"
#include <Utilities/State.h>
#include <glm/gtx/transform.hpp>
#include <GLFW/glfw3.h>
#include <chrono>

void SceneBase::OnKey(int key, int action)
{
    if (action == GLFW_PRESS)
        m_keys[key] = true;
    else if (action == GLFW_RELEASE)
        m_keys[key] = false;

    if (key == GLFW_KEY_N && action == GLFW_PRESS)
    {
        auto & state = CurState<bool>::Instance().state;
        state["disable_norm"] = !state["disable_norm"];
    }
}

void SceneBase::OnMouse(bool first_event, double xpos, double ypos)
{
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

void SceneBase::UpdateAngle()
{
    static std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
    std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
    int64_t elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    start = end;
    //angle += elapsed / 2e6f;
}

void SceneBase::UpdateCameraMovement()
{
    double currentFrame = glfwGetTime();
    m_delta_time = (float)(currentFrame - m_last_frame);
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

void SceneBase::UpdateCBuffers(IShaderBuffer & constant_buffer_geometry_pass, IShaderBuffer & constant_buffer_light_pass)
{
    glm::mat4 projection, view, model;
    m_camera.GetMatrix(projection, view, model);

    float model_scale_ = 0.01f;
    model = glm::scale(glm::vec3(model_scale_)) * model;

    constant_buffer_geometry_pass.Uniform("model") = StoreMatrix(model);
    constant_buffer_geometry_pass.Uniform("view") = StoreMatrix(view);
    constant_buffer_geometry_pass.Uniform("projection") = StoreMatrix(projection);
    constant_buffer_geometry_pass.Uniform("normalMatrix") = StoreMatrix(glm::transpose(glm::inverse(model)));

    float light_r = 2.5;
    glm::vec3 light_pos_ = glm::vec3(light_r * cos(m_angle), 25.0f, light_r * sin(m_angle));

    glm::vec3 cameraPosView = glm::vec3(glm::vec4(m_camera.GetCameraPos(), 1.0));
    glm::vec3 lightPosView = glm::vec3(glm::vec4(light_pos_, 1.0));

    constant_buffer_light_pass.Uniform("lightPos") = glm::vec4(lightPosView, 0.0);
    constant_buffer_light_pass.Uniform("viewPos") = glm::vec4(cameraPosView, 0.0);
}

void SceneBase::SetLight(IShaderBuffer & light_buffer_geometry_pass)
{
    light_buffer_geometry_pass.Uniform("light_ambient") = glm::vec3(0.2f);
    light_buffer_geometry_pass.Uniform("light_diffuse") = glm::vec3(1.0f);
    light_buffer_geometry_pass.Uniform("light_specular") = glm::vec3(0.5f);
}

void SceneBase::SetMaterial(IShaderBuffer & material_buffer_geometry_pass, IMesh & cur_mesh)
{
    material_buffer_geometry_pass.Uniform("material_ambient") = cur_mesh.material.amb;
    material_buffer_geometry_pass.Uniform("material_diffuse") = cur_mesh.material.dif;
    material_buffer_geometry_pass.Uniform("material_specular") = cur_mesh.material.spec;
    material_buffer_geometry_pass.Uniform("material_shininess") = cur_mesh.material.shininess;
}

std::vector<int> SceneBase::MapTextures(IMesh & cur_mesh, IShaderBuffer & textures_enables)
{
    std::vector<int> use_textures(6, -1);

    textures_enables.Uniform("has_diffuseMap") = 0;
    textures_enables.Uniform("has_normalMap") = 0;
    textures_enables.Uniform("has_specularMap") = 0;
    textures_enables.Uniform("has_glossMap") = 0;
    textures_enables.Uniform("has_ambientMap") = 0;
    textures_enables.Uniform("has_alphaMap") = 0;

    for (size_t i = 0; i < cur_mesh.textures.size(); ++i)
    {
        uint32_t texture_slot = 0;
        switch (cur_mesh.textures[i].type)
        {
        case aiTextureType_HEIGHT:
        {
            texture_slot = 0;
            auto& state = CurState<bool>::Instance().state;
            if (!state["disable_norm"])
                textures_enables.Uniform("has_normalMap") = 1;
        } break;
        case aiTextureType_OPACITY:
            texture_slot = 1;
            textures_enables.Uniform("has_alphaMap") = 1;
            break;
        case aiTextureType_AMBIENT:
            texture_slot = 2;
            textures_enables.Uniform("has_ambientMap") = 1;
            break;
        case aiTextureType_DIFFUSE:
            texture_slot = 3;
            textures_enables.Uniform("has_diffuseMap") = 1;
            break;
        case aiTextureType_SPECULAR:
            texture_slot = 4;
            textures_enables.Uniform("has_specularMap") = 1;
            break;
        case aiTextureType_SHININESS:
            texture_slot = 5;
            textures_enables.Uniform("has_glossMap") = 1;
            break;
        default:
            continue;
        }
        use_textures[texture_slot] = i;
    }
    return use_textures;
}
