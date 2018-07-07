#include "Context/Context.h"

Context::Context(GLFWwindow* window, int width, int height)
    : m_window(window)
    , m_width(width)
    , m_height(height)
{
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

uint32_t Context::GetWorkaroundAssimpFlags()
{
    return ~0;
}

glm::mat4 Context::GetClipMatrix()
{
    return glm::mat4(1);
}
