#include "Context/Context.h"
#include <Utilities/DXUtility.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

Context::Context(GLFWwindow* window, int width, int height)
    : window(window)
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
