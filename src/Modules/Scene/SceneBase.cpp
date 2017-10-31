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
