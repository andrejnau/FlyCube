#include "AppBox/AppBox.h"
#include <GLFW/glfw3.h>
#include <sstream>
#include <Utilities/State.h>

AppBox::AppBox(int argc, char* argv[], const std::string& title, ApiType& api_type)
    : m_api_type(api_type)
    , m_window(nullptr)
    , m_width(0)
    , m_height(0)
    , m_exit(false)
{
    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);
        if (arg == "--dx12")
            m_api_type = ApiType::kDX12;
        else if (arg == "--vk")
            m_api_type = ApiType::kVulkan;
        else if (arg == "--gpu")
            CurState::Instance().required_gpu_index = std::stoul(argv[++i]);
        else if (arg == "--no_vsync")
            CurState::Instance().vsync = false;
        else if (arg == "--force_dxil")
            CurState::Instance().force_dxil = true;
    }

    std::string api_title;
    switch (m_api_type)
    {
    case ApiType::kDX12:
        api_title = "[DX12]";
        break;
    case ApiType::kVulkan:
        api_title = "[Vulkan]";
        break;
    }
    m_title = api_title + " " + title;

    auto monitor_desc = GetPrimaryMonitorRect();
    m_width = monitor_desc.width / 1.5;
    m_height = monitor_desc.height / 1.5;

    Init();
}

AppBox::~AppBox()
{
    if (m_window)
    {
        m_sample.reset();
        glfwDestroyWindow(m_window);
    }

    glfwTerminate();
}

int AppBox::Run()
{
    if (!m_window)
        return EXIT_FAILURE;

    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();

        if (m_exit)
            break;

        if (m_sample)
        {
            m_sample->OnUpdate();
            m_sample->OnRender();
        }

        UpdateFps();
    }

    return EXIT_SUCCESS;
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
            if (CurState::Instance().round_fps)
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

bool AppBox::ShouldClose()
{
    return glfwWindowShouldClose(m_window) || m_exit;
}

void AppBox::PollEvents()
{
    glfwPollEvents();
}

AppRect AppBox::GetAppRect() const
{
    return { m_width, m_height };
}

GLFWwindow* AppBox::GetWindow() const
{
    return m_window;
}

AppRect AppBox::GetPrimaryMonitorRect()
{
    glfwInit();
    auto monitor_desc = *glfwGetVideoMode(glfwGetPrimaryMonitor());
    return { monitor_desc.width, monitor_desc.height };
}

void AppBox::Init()
{
    if (!glfwInit())
        return;

    InitWindow();
    SetWindowToCenter();
}

void AppBox::InitWindow()
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
    if (!m_window)
        return;

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, AppBox::OnSizeChanged);
    glfwSetKeyCallback(m_window, AppBox::OnKey);
    glfwSetCursorPosCallback(m_window, AppBox::OnMouse);
    glfwSetMouseButtonCallback(m_window, AppBox::OnMouseButton);
    glfwSetScrollCallback(m_window, AppBox::OnScroll);
    glfwSetCharCallback(m_window, AppBox::OnInputChar);
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void AppBox::SetWindowToCenter()
{
    int window_width, window_height;
    glfwGetWindowSize(m_window, &window_width, &window_height);

    const GLFWvidmode* monitor_vidmode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int monitor_width = monitor_vidmode->width;
    int monitor_height = monitor_vidmode->height;

    int xpos = (monitor_width - window_width) / 2;
    int ypos = (monitor_height - window_height) / 2;

    glfwSetWindowPos(m_window, xpos, ypos);
}

void AppBox::OnSizeChanged(GLFWwindow* window, int width, int height)
{
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(window));
    self->m_width = width;
    self->m_height = height;
    if (self->m_sample)
        self->m_sample->OnResize(width, height);
}

void AppBox::OnKey(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(window));

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        self->m_exit = true;

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
    {
        if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        else
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    if (self->m_sample)
        self->m_sample->OnKey(key, action);
}

void AppBox::OnMouse(GLFWwindow* window, double xpos, double ypos)
{
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(window));
    if (!self->m_sample)
        return;
    static bool first_event = true;
    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
    {
        self->m_sample->OnMouse(first_event, xpos, ypos);
        first_event = false;
    }
    else
    {
        self->m_sample->OnMouse(first_event, xpos, ypos);
        first_event = true;
    }
}

void AppBox::OnMouseButton(GLFWwindow* window, int button, int action, int mods)
{
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(window));
    if (!self->m_sample)
        return;
    self->m_sample->OnMouseButton(button, action);
}

void AppBox::OnScroll(GLFWwindow* window, double xoffset, double yoffset)
{
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(window));
    if (!self->m_sample)
        return;
    self->m_sample->OnScroll(xoffset, yoffset);
}

void AppBox::OnInputChar(GLFWwindow* window, unsigned int ch)
{
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(window));
    if (!self->m_sample)
        return;
    self->m_sample->OnInputChar(ch);
}
