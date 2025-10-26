#include "AppBox/AppBox.h"

#include "WindowUtils/WindowUtils.h"

#if defined(__APPLE__)
#include "AppBox/AutoreleasePool.h"
#endif

#include <format>

AppBox::AppBox(const std::string_view& title, const Settings& setting)
    : m_setting(setting)
{
    std::string api_str;
    switch (setting.api_type) {
    case ApiType::kDX12:
        api_str = "[DX12]";
        break;
    case ApiType::kVulkan:
        api_str = "[Vulkan]";
        break;
    case ApiType::kMetal:
        api_str = "[Metal]";
        break;
    }
    m_title = api_str + " ";
    m_title += title;

    glfwInit();

    m_window = WindowUtils::CreateWindowWithDefaultSize(m_title);
    m_size = WindowUtils::GetSurfaceSize(m_window);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetWindowSizeCallback(m_window, AppBox::OnSizeChanged);
    glfwSetKeyCallback(m_window, AppBox::OnKey);
    glfwSetCursorPosCallback(m_window, AppBox::OnMouse);
    glfwSetMouseButtonCallback(m_window, AppBox::OnMouseButton);
    glfwSetScrollCallback(m_window, AppBox::OnScroll);
    glfwSetCharCallback(m_window, AppBox::OnInputChar);
    glfwSetInputMode(m_window, GLFW_CURSOR, m_mouse_mode);

#if defined(__APPLE__)
    m_autorelease_pool = CreateAutoreleasePool();
#endif
}

AppBox::~AppBox()
{
    if (m_window) {
        glfwDestroyWindow(m_window);
    }
    glfwTerminate();
}

void AppBox::SetGpuName(const std::string& gpu_name)
{
    m_gpu_name = gpu_name;
}

void AppBox::UpdateFps()
{
    ++m_frame_number;
    double current_time = glfwGetTime();
    double delta = current_time - m_last_time;
    if (delta > 1.0) {
        if (m_last_time > 0.0) {
            m_fps = std::format("({} FPS)", static_cast<int64_t>(std::round(m_frame_number / delta)));
        }
        m_frame_number = 0;
        m_last_time = current_time;
    }
}

void AppBox::UpdateTitle()
{
    std::string title = m_title;
    if (!m_gpu_name.empty()) {
        title += " " + m_gpu_name;
    }
    UpdateFps();
    if (!m_fps.empty()) {
        title += " " + m_fps;
    }

    glfwSetWindowTitle(m_window, title.c_str());
}

void AppBox::SubscribeEvents(InputEvents* input_listener, WindowEvents* window_listener)
{
    m_input_listener = input_listener;
    m_window_listener = window_listener;
}

bool AppBox::PollEvents()
{
#if defined(__APPLE__)
    m_autorelease_pool->Reset();
#endif
    UpdateTitle();
    glfwPollEvents();
    return glfwWindowShouldClose(m_window) || m_exit_request;
}

AppSize AppBox::GetAppSize() const
{
    return m_size;
}

GLFWwindow* AppBox::GetWindow() const
{
    return m_window;
}

void* AppBox::GetNativeWindow() const
{
    return WindowUtils::GetNativeWindow(m_window);
}

void AppBox::SwitchFullScreenMode()
{
    if (glfwGetWindowMonitor(m_window)) {
        glfwSetWindowMonitor(m_window, nullptr, m_window_box[0], m_window_box[1], m_window_box[2], m_window_box[3], 0);
    } else {
        glfwGetWindowPos(m_window, &m_window_box[0], &m_window_box[1]);
        glfwGetWindowSize(m_window, &m_window_box[2], &m_window_box[3]);
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }
}

void AppBox::OnSizeChanged(GLFWwindow* m_window, int width, int height)
{
    if (width == 0 && height == 0) {
        return;
    }
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(m_window));
    self->m_size = AppSize(width, height);
    if (self->m_window_listener) {
        self->m_window_listener->OnResize(width, height);
    }
}

void AppBox::OnKey(GLFWwindow* m_window, int key, int scancode, int action, int mods)
{
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(m_window));
    self->m_keys[key] = action;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        self->m_exit_request = true;
    }

    if (self->m_keys[GLFW_KEY_RIGHT_ALT] == GLFW_PRESS && self->m_keys[GLFW_KEY_ENTER] == GLFW_PRESS) {
        self->SwitchFullScreenMode();
    }

    if (key == GLFW_KEY_L && action == GLFW_PRESS) {
        if (self->m_mouse_mode == GLFW_CURSOR_HIDDEN) {
            self->m_mouse_mode = GLFW_CURSOR_DISABLED;
        } else {
            self->m_mouse_mode = GLFW_CURSOR_HIDDEN;
        }

        if (glfwGetInputMode(m_window, GLFW_CURSOR) != GLFW_CURSOR_NORMAL) {
            glfwSetInputMode(m_window, GLFW_CURSOR, self->m_mouse_mode);
        }
    }

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        if (glfwGetInputMode(m_window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL) {
            glfwSetInputMode(m_window, GLFW_CURSOR, self->m_mouse_mode);
        } else {
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    if (self->m_input_listener) {
        self->m_input_listener->OnKey(key, action);
    }
}

void AppBox::OnMouse(GLFWwindow* m_window, double xpos, double ypos)
{
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(m_window));
    if (!self->m_input_listener) {
        return;
    }
    static bool first_event = true;
    if (glfwGetInputMode(m_window, GLFW_CURSOR) != GLFW_CURSOR_NORMAL) {
        self->m_input_listener->OnMouse(first_event, xpos, ypos);
        first_event = false;
    } else {
        self->m_input_listener->OnMouse(first_event, xpos, ypos);
        first_event = true;
    }
}

void AppBox::OnMouseButton(GLFWwindow* m_window, int button, int action, int mods)
{
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(m_window));
    if (!self->m_input_listener) {
        return;
    }
    self->m_input_listener->OnMouseButton(button, action);
}

void AppBox::OnScroll(GLFWwindow* m_window, double xoffset, double yoffset)
{
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(m_window));
    if (!self->m_input_listener) {
        return;
    }
    self->m_input_listener->OnScroll(xoffset, yoffset);
}

void AppBox::OnInputChar(GLFWwindow* m_window, uint32_t ch)
{
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(m_window));
    if (!self->m_input_listener) {
        return;
    }
    self->m_input_listener->OnInputChar(ch);
}
