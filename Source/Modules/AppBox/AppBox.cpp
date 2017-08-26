#include "AppBox.h"
#include <GLFW/glfw3.h>

void SetWindowToCenter(GLFWwindow* window)
{
    int window_width, window_height;
    glfwGetWindowSize(window, &window_width, &window_height);

    int monitor_width, monitor_height;
    const GLFWvidmode *monitor_vidmode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    monitor_width = monitor_vidmode->width;
    monitor_height = monitor_vidmode->height;

    glfwSetWindowPos(window,
        (monitor_width - window_width) / 2,
        (monitor_height - window_height) / 2);
}

AppBox::AppBox(ISample& sample, const std::string& title, int width, int height)
    : m_sample(sample)
    , m_window(nullptr)
    , m_width(width)
    , m_height(height)
{
    if (!glfwInit())
        return;

    InitWindow(title, width, height);
    SetWindowToCenter(m_window);
}

AppBox::~AppBox()
{
    if (m_window)
    {
        glfwDestroyWindow(m_window);
        m_sample.OnDestroy();
    }

    glfwTerminate();
}

int AppBox::Run()
{
    if (!m_window)
        return EXIT_FAILURE;

    m_sample.OnInit(m_width, m_height);

    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();

        m_sample.OnUpdate();
        m_sample.OnRender();
    }

    return EXIT_SUCCESS;
}

void AppBox::InitWindow(const std::string& title, int width, int height)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_window)
        return;

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, AppBox::OnSizeChanged);
}

void AppBox::OnSizeChanged(GLFWwindow * window, int width, int height)
{
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(window));
    self->m_width = width;
    self->m_height = height;
    self->m_sample.OnSizeChanged(width, height);
}
