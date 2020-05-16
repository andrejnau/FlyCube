#include "AppBox/AppBox.h"
#include <sstream>
#include <cassert>

AppBox::AppBox(const std::string& title, Settings setting)
    : m_setting(setting)
{
    std::string api_str;
    switch (setting.api_type)
    {
    case ApiType::kDX12:
        api_str = "[DX12]";
        break;
    case ApiType::kVulkan:
        api_str = "[Vulkan]";
        break;
    }
    m_title = api_str + " " + title;

    glfwInit();

    const GLFWvidmode* monitor_desc = glfwGetVideoMode(glfwGetPrimaryMonitor());
    m_width = static_cast<int>(monitor_desc->width / 1.5);
    m_height = static_cast<int>(monitor_desc->height / 1.5);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, AppBox::OnSizeChanged);
    glfwSetKeyCallback(m_window, AppBox::OnKey);
    glfwSetCursorPosCallback(m_window, AppBox::OnMouse);
    glfwSetMouseButtonCallback(m_window, AppBox::OnMouseButton);
    glfwSetScrollCallback(m_window, AppBox::OnScroll);
    glfwSetCharCallback(m_window, AppBox::OnInputChar);
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    int xpos = (monitor_desc->width - m_width) / 2;
    int ypos = (monitor_desc->height - m_height) / 2;
    glfwSetWindowPos(m_window, xpos, ypos);
}

AppBox::~AppBox()
{
    if (m_window)
        glfwDestroyWindow(m_window);
    glfwTerminate();
}

void AppBox::UpdateFps()
{
    ++m_frame_number;
    double current_time = glfwGetTime();
    double delta = current_time - m_last_time;
    double eps = 1e-6;
    if (delta + eps > 1.0)
    {
        if (m_last_time > 0)
        {
            double fps = m_frame_number / delta;
            std::stringstream buf;
            buf << m_title << " [";
            if (m_setting.round_fps)
                buf << static_cast<int64_t>(std::round(fps));
            else
                buf << fps;
            buf << " FPS]";
            glfwSetWindowTitle(m_window, buf.str().c_str());
        }

        m_frame_number = 0;
        m_last_time = current_time;
    }
}

bool AppBox::PollEvents()
{
    glfwPollEvents();
    return glfwWindowShouldClose(m_window) || m_exit_request;
}

AppRect AppBox::GetAppRect() const
{
    return { m_width, m_height };
}

GLFWwindow* AppBox::GetWindow() const
{
    return m_window;
}

void AppBox::OnSizeChanged(GLFWwindow* m_window, int width, int height)
{
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(m_window));
    self->m_width = width;
    self->m_height = height;
    if (self->m_window_listener)
        self->m_window_listener->OnResize(width, height);
}

void AppBox::OnKey(GLFWwindow* m_window, int key, int scancode, int action, int mods)
{
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(m_window));

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        self->m_exit_request = true;

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
    {
        if (glfwGetInputMode(m_window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        else
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    if (self->m_input_listener)
        self->m_input_listener->OnKey(key, action);
}

void AppBox::OnMouse(GLFWwindow* m_window, double xpos, double ypos)
{
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(m_window));
    if (!self->m_input_listener)
        return;
    static bool first_event = true;
    if (glfwGetInputMode(m_window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
    {
        self->m_input_listener->OnMouse(first_event, xpos, ypos);
        first_event = false;
    }
    else
    {
        self->m_input_listener->OnMouse(first_event, xpos, ypos);
        first_event = true;
    }
}

void AppBox::OnMouseButton(GLFWwindow* m_window, int button, int action, int mods)
{
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(m_window));
    if (!self->m_input_listener)
        return;
    self->m_input_listener->OnMouseButton(button, action);
}

void AppBox::OnScroll(GLFWwindow* m_window, double xoffset, double yoffset)
{
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(m_window));
    if (!self->m_input_listener)
        return;
    self->m_input_listener->OnScroll(xoffset, yoffset);
}

void AppBox::OnInputChar(GLFWwindow* m_window, uint32_t ch)
{
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(m_window));
    if (!self->m_input_listener)
        return;
    self->m_input_listener->OnInputChar(ch);
}
