#pragma once

#include <ISample/ISample.h>
#include <GLFW/glfw3.h>
#include <string>

class AppBox
{
public:
    AppBox(ISample& sample, const std::string& title, int width, int height);
    ~AppBox();
    int Run();
private:
    void InitWindow(const std::string& title, int width, int height);

    static void OnSizeChanged(GLFWwindow* window, int width, int height);

    ISample& m_sample;
    GLFWwindow* m_window;
    int m_width;
    int m_height;
};
