#include "Context/Context.h"
#include <Utilities/State.h>

Context::Context(GLFWwindow* window)
    : m_window(window)
    , m_width(0)
    , m_height(0)
{
    glfwGetWindowSize(window, &m_width, &m_height);
}

void Context::OnResize(int width, int height)
{
    m_width = width;
    m_height = height;
    ResizeBackBuffer(m_width, m_height);
}

size_t Context::GetFrameIndex() const
{
    return m_frame_index;
}

GLFWwindow* Context::GetWindow()
{
    return m_window;
}

Resource::Ptr Context::CreateBottomLevelAS(const BufferDesc & vertex)
{
    return Resource::Ptr();
}

Resource::Ptr Context::CreateBottomLevelAS(const BufferDesc & vertex, const BufferDesc & index)
{
    return Resource::Ptr();
}

Resource::Ptr Context::CreateTopLevelAS(const std::vector<std::pair<Resource::Ptr, glm::mat4>>& geometry)
{
    return Resource::Ptr();
}
