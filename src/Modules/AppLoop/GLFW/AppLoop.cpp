#include "AppLoop/AppLoop.h"

#include "AppLoop/GLFW/WindowUtils.h"

#include <cassert>

// static
AppLoop& AppLoop::GetInstance()
{
    static AppLoop instance;
    return instance;
}

AppRenderer& AppLoop::GetRendererImpl()
{
    assert(m_renderer);
    return *m_renderer;
}

int AppLoop::RunImpl(std::unique_ptr<AppRenderer> renderer, int argc, char* argv[])
{
    glfwInit();
    GLFWwindow* window = CreateWindowWithDefaultSize(renderer->GetTitle());

    renderer->Init(GetSurfaceSize(window), GetNativeWindow(window));
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        renderer->Render();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
