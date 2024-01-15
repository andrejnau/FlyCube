#include "AppBox/AppBox.h"

#include <cassert>
#include <cmath>
#include <sstream>
#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#else
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3native.h>

#if defined(__APPLE__)
#import <QuartzCore/QuartzCore.h>
#endif

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

    const GLFWvidmode* monitor_desc = glfwGetVideoMode(glfwGetPrimaryMonitor());
    m_width = static_cast<uint32_t>(monitor_desc->width / 1.5);
    m_height = static_cast<uint32_t>(monitor_desc->height / 1.5);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetWindowSizeCallback(m_window, AppBox::OnSizeChanged);
    glfwSetKeyCallback(m_window, AppBox::OnKey);
    glfwSetCursorPosCallback(m_window, AppBox::OnMouse);
    glfwSetMouseButtonCallback(m_window, AppBox::OnMouseButton);
    glfwSetScrollCallback(m_window, AppBox::OnScroll);
    glfwSetCharCallback(m_window, AppBox::OnInputChar);
    glfwSetInputMode(m_window, GLFW_CURSOR, m_mouse_mode);

    int xpos = (monitor_desc->width - m_width) / 2;
    int ypos = (monitor_desc->height - m_height) / 2;
    glfwSetWindowPos(m_window, xpos, ypos);
    glfwGetFramebufferSize(m_window, (int*)&m_width, (int*)&m_height);

#if defined(__APPLE__)
    m_autorelease_pool = CreateAutoreleasePool();

    NSWindow* nswindow = glfwGetCocoaWindow(m_window);
    nswindow.contentView.layer = [CAMetalLayer layer];
    nswindow.contentView.wantsLayer = YES;
    m_layer = (__bridge void*)nswindow.contentView.layer;
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
    std::stringstream buf;
    ++m_frame_number;
    double current_time = glfwGetTime();
    double delta = current_time - m_last_time;
    double eps = 1e-6;
    if (delta + eps > 1.0) {
        std::stringstream buf;
        if (m_last_time > 0) {
            double fps = m_frame_number / delta;
            buf << "(";
            buf << static_cast<int64_t>(std::round(fps));
            buf << " FPS)";
            m_fps = buf.str();
        }
        m_frame_number = 0;
        m_last_time = current_time;
    }
}

void AppBox::UpdateTitle()
{
    std::stringstream buf;
    buf << m_title;

    if (!m_gpu_name.empty()) {
        buf << " " << m_gpu_name;
    }

    UpdateFps();

    if (!m_fps.empty()) {
        buf << " " << m_fps;
    }

    glfwSetWindowTitle(m_window, buf.str().c_str());
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
    return { m_width, m_height };
}

GLFWwindow* AppBox::GetWindow() const
{
    return m_window;
}

void* AppBox::GetNativeWindow() const
{
#if defined(_WIN32)
    return glfwGetWin32Window(m_window);
#elif defined(__APPLE__)
    return m_layer;
#else
    return (void*)glfwGetX11Window(m_window);
#endif
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
    self->m_width = width;
    self->m_height = height;
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
