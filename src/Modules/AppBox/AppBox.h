#pragma once

#include <Scene/IScene.h>
#include <GLFW/glfw3.h>
#include <functional>
#include <string>
#include <memory>
#include <Context/Context.h>

class AppBox
{
public:
    using CreateSample = std::function<IScene::Ptr(Context&, int, int)>;
    AppBox(const CreateSample& create_sample, ApiType api_type, const std::string& title, int width, int height);
    ~AppBox();
    int Run();

    struct MonitorDesc
    {
        int width;
        int height;
    };

    static MonitorDesc GetPrimaryMonitorDesc();

private:
    void InitWindow();
    void SetWindowToCenter();

    static void OnSizeChanged(GLFWwindow* window, int width, int height);
    static void OnKey(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void OnMouse(GLFWwindow* window, double xpos, double ypos);
    static void OnMouseButton(GLFWwindow* window, int button, int action, int mods);
    static void OnScroll(GLFWwindow* window, double xoffset, double yoffset);
    static void OnInputChar(GLFWwindow* window, unsigned int ch);

    CreateSample m_create_sample;
    ApiType m_api_type;
    std::string m_title;
    IScene::Ptr m_sample;
    GLFWwindow* m_window;
    int m_width;
    int m_height;
    bool m_exit;
    std::unique_ptr<Context> m_context;
};
