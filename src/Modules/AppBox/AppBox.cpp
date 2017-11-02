#include "AppBox/AppBox.h"
#include <GLFW/glfw3.h>
#include <sstream>

AppBox::AppBox(const CreateSample& create_sample, ApiType api_type, const std::string& title, int width, int height)
    : m_create_sample(create_sample)
    , m_api_type(api_type)
    , m_title(title)
    , m_window(nullptr)
    , m_width(width)
    , m_height(height)
{
    if (!glfwInit())
        return;

    InitWindow();
    SetWindowToCenter();
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

    m_sample = m_create_sample(m_width, m_height);

    int frame_number = 0;
    double last_time = glfwGetTime();
    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();

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

void AppBox::InitWindow()
{
    if (m_api_type == ApiType::kOpenGL)
    {
        glfwWindowHint(GLFW_SAMPLES, 4);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
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
        exit(EXIT_SUCCESS);

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
        first_event = true;
    }
}

void AppBox::OnMouseButton(GLFWwindow * window, int button, int action, int mods)
{
}
