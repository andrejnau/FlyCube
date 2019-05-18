#include "AppBox/AppBox.h"
#include <GLFW/glfw3.h>
#include <sstream>
#include <Context/ContextSelector.h>
#include <Utilities/State.h>

AppBox::AppBox(int argc, char *argv[], const CreateSample& create_sample, const std::string& title)
    : m_create_sample(create_sample)
    , m_api_type(ApiType::kDX12)
    , m_window(nullptr)
    , m_width(0)
    , m_height(0)
    , m_exit(false)
{
    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);
        if (arg == "--dx11")
            m_api_type = ApiType::kDX11;
        else if (arg == "--dx12")
            m_api_type = ApiType::kDX12;
        else if (arg == "--vk")
            m_api_type = ApiType::kVulkan;
        else if (arg == "--gl")
            m_api_type = ApiType::kOpenGL;
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
    case ApiType::kDX11:
        api_title = "[DX11]";
        break;
    case ApiType::kDX12:
        api_title = "[DX12]";
        break;
    case ApiType::kVulkan:
        api_title = "[Vulkan]";
        break;
    case ApiType::kOpenGL:
        api_title = "[OpenGL]";
        break;
    }
    m_title = api_title + " " + title;

    auto monitor_desc = GetPrimaryMonitorDesc();
    m_width = monitor_desc.width / 1.5;
    m_height = monitor_desc.height / 1.5;

    Init();
}

AppBox::AppBox(const CreateSample& create_sample, ApiType api_type, const std::string& title, int width, int height)
    : m_create_sample(create_sample)
    , m_api_type(api_type)
    , m_title(title)
    , m_window(nullptr)
    , m_width(width)
    , m_height(height)
    , m_exit(false)
{
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

    m_context = CreateContext(m_api_type, m_window);
    m_sample = m_create_sample(*m_context, m_width, m_height);

    int frame_number = 0;
    double last_time = glfwGetTime();
    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();

        if (m_exit)
            break;

        m_sample->OnUpdate();
        m_sample->OnRender();

        if (m_api_type == ApiType::kOpenGL)
        {
            glfwSwapBuffers(m_window);
        }

        ++frame_number;
        double current_time = glfwGetTime();
        double delta = current_time - last_time;
        if (delta >= 1.0)
        {
            double fps = double(frame_number) / delta;

            std::stringstream ss;
            ss << m_title<< " [" << fps << " FPS]";

            glfwSetWindowTitle(m_window, ss.str().c_str());

            frame_number = 0;
            last_time = current_time;
        }
    }

    return EXIT_SUCCESS;
}

AppBox::MonitorDesc AppBox::GetPrimaryMonitorDesc()
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
    if (m_api_type == ApiType::kOpenGL)
    {
        glfwWindowHint(GLFW_SAMPLES, 4);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    }
    else
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
    if (!m_window)
        return;

    if (m_api_type == ApiType::kOpenGL)
    {
        glfwMakeContextCurrent(m_window);
    }

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

    self->m_sample->OnKey(key, action);
}

void AppBox::OnMouse(GLFWwindow* window, double xpos, double ypos)
{
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(window));
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
    self->m_sample->OnMouseButton(button, action);
}

void AppBox::OnScroll(GLFWwindow* window, double xoffset, double yoffset)
{
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(window));
    self->m_sample->OnScroll(xoffset, yoffset);
}

void AppBox::OnInputChar(GLFWwindow* window, unsigned int ch)
{
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(window));
    self->m_sample->OnInputChar(ch);
}
