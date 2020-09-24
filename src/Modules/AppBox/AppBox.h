#pragma once
#include "AppBox/InputEvents.h"
#include "AppBox/WindowEvents.h"
#include "AppBox/Settings.h"
#include <ApiType/ApiType.h>
#include <functional>
#include <string>
#include <memory>
#include <array>
#include <map>
#include <GLFW/glfw3.h>

struct AppRect
{
    uint32_t width;
    uint32_t height;
};

class AppBox
{
public:
    AppBox(const std::string& title, Settings setting);
    ~AppBox();
    bool PollEvents();

    AppRect GetAppRect() const;
    GLFWwindow* GetWindow() const;
    void UpdateFps(const std::string& gpu_name);
    void SubscribeEvents(InputEvents* input_listener, WindowEvents* window_listener);

private:
    void SwitchFullScreenMode();
    static void OnSizeChanged(GLFWwindow* window, int width, int height);
    static void OnKey(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void OnMouse(GLFWwindow* window, double xpos, double ypos);
    static void OnMouseButton(GLFWwindow* window, int button, int action, int mods);
    static void OnScroll(GLFWwindow* window, double xoffset, double yoffset);
    static void OnInputChar(GLFWwindow* window, uint32_t ch);

    Settings m_setting;
    std::string m_title;
    InputEvents* m_input_listener = nullptr;
    WindowEvents* m_window_listener = nullptr;
    GLFWwindow* m_window = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    bool m_exit_request = false;
    uint32_t m_frame_number = 0;
    double m_last_time = 0;
    std::array<int, 4> m_window_box = {};
    std::map<int, bool> m_keys;
    int m_mouse_mode = GLFW_CURSOR_HIDDEN;
};
