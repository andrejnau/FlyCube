#include "Scene/SceneBase.h"
#include <Utilities/State.h>
#include <glm/gtx/transform.hpp>
#include <GLFW/glfw3.h>
#include <chrono>

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
