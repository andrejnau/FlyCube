#pragma once
#include "AppBox/InputEvents.h"
#include "AppBox/WindowEvents.h"
#include "AppLoop/AppSize.h"
#include "AppSettings/Settings.h"
#include "Instance/BaseTypes.h"

#include <GLFW/glfw3.h>

#include <array>
#include <map>
#include <memory>
#include <string>
#include <string_view>

enum class CursorMode {
    kNormal,
    kHidden,
    kDisabled,
};

class AppBox {
public:
    AppBox(const std::string_view& title, const Settings& setting);
    ~AppBox();
    bool PollEvents();

    AppSize GetAppSize() const;
    GLFWwindow* GetWindow() const;
    NativeSurface GetNativeSurface() const;
    void SetGpuName(const std::string& gpu_name);
    void SubscribeEvents(InputEvents* input_listener, WindowEvents* window_listener);
    const CursorMode& GetCursorMode() const;
    void SetCursorMode(CursorMode cursor_mode);

private:
    void UpdateFps();
    void UpdateTitle();
    void SwitchFullScreenMode();
    static void OnSizeChanged(GLFWwindow* window, int width, int height);
    static void OnKey(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void OnMouse(GLFWwindow* window, double xpos, double ypos);
    static void OnMouseButton(GLFWwindow* window, int button, int action, int mods);
    static void OnScroll(GLFWwindow* window, double xoffset, double yoffset);
    static void OnInputChar(GLFWwindow* window, uint32_t ch);

    Settings setting_;
    std::string title_;
    InputEvents* input_listener_ = nullptr;
    WindowEvents* window_listener_ = nullptr;
    GLFWwindow* window_ = nullptr;
    AppSize size_;
    bool exit_request_ = false;
    uint32_t frame_number_ = 0;
    double last_time_ = 0;
    std::array<int, 4> window_box_ = {};
    std::map<int, bool> keys_;
    CursorMode cursor_mode_ = CursorMode::kNormal;
    std::string gpu_name_;
    std::string fps_;
#if defined(__APPLE__)
    std::shared_ptr<class AutoreleasePool> autorelease_pool_;
#endif
};
