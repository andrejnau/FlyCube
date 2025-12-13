#include "AppBox/AppBox.h"

#include "AppBox/WindowUtils.h"
#include "Utilities/NotReached.h"

#if defined(__APPLE__)
#include "AppBox/AutoreleasePool.h"
#endif

#include <cmath>
#include <format>

namespace {

int ConvertCursorMode(CursorMode mode)
{
    switch (mode) {
    case CursorMode::kNormal:
        return GLFW_CURSOR_NORMAL;
    case CursorMode::kHidden:
        return GLFW_CURSOR_HIDDEN;
    case CursorMode::kDisabled:
        return GLFW_CURSOR_DISABLED;
    default:
        NOTREACHED();
    }
}

} // namespace

AppBox::AppBox(const std::string_view& title, const Settings& setting)
    : setting_(setting)
{
    std::string api_str;
    switch (setting.api_type) {
    case ApiType::kDX12:
        api_str = "[DX12]";
        break;
    case ApiType::kVulkan:
        api_str = "[Vulkan]";
        break;
    case ApiType::kMetal:
        api_str = "[Metal]";
        break;
    }
    title_ = api_str + " ";
    title_ += title;

    glfwInit();

    window_ = WindowUtils::CreateWindowWithDefaultSize(title_);
    size_ = WindowUtils::GetSurfaceSize(window_);
    glfwSetWindowUserPointer(window_, this);
    glfwSetWindowSizeCallback(window_, AppBox::OnSizeChanged);
    glfwSetKeyCallback(window_, AppBox::OnKey);
    glfwSetCursorPosCallback(window_, AppBox::OnMouse);
    glfwSetMouseButtonCallback(window_, AppBox::OnMouseButton);
    glfwSetScrollCallback(window_, AppBox::OnScroll);
    glfwSetCharCallback(window_, AppBox::OnInputChar);
    glfwSetInputMode(window_, GLFW_CURSOR, ConvertCursorMode(cursor_mode_));

#if defined(__APPLE__)
    autorelease_pool_ = CreateAutoreleasePool();
#endif
}

AppBox::~AppBox()
{
    if (window_) {
        glfwDestroyWindow(window_);
    }
    glfwTerminate();
}

void AppBox::SetGpuName(const std::string& gpu_name)
{
    gpu_name_ = gpu_name;
}

void AppBox::UpdateFps()
{
    ++frame_number_;
    double current_time = glfwGetTime();
    double delta = current_time - last_time_;
    if (delta > 1.0) {
        if (last_time_ > 0.0) {
            fps_ = std::format("({} FPS)", static_cast<int64_t>(std::round(frame_number_ / delta)));
        }
        frame_number_ = 0;
        last_time_ = current_time;
    }
}

void AppBox::UpdateTitle()
{
    std::string title = title_;
    if (!gpu_name_.empty()) {
        title += " " + gpu_name_;
    }
    UpdateFps();
    if (!fps_.empty()) {
        title += " " + fps_;
    }

    glfwSetWindowTitle(window_, title.c_str());
}

void AppBox::SubscribeEvents(InputEvents* input_listener, WindowEvents* window_listener)
{
    input_listener_ = input_listener;
    window_listener_ = window_listener;
}

const CursorMode& AppBox::GetCursorMode() const
{
    return cursor_mode_;
}

void AppBox::SetCursorMode(CursorMode cursor_mode)
{
    cursor_mode_ = cursor_mode;
    glfwSetInputMode(window_, GLFW_CURSOR, ConvertCursorMode(cursor_mode_));
}

bool AppBox::PollEvents()
{
#if defined(__APPLE__)
    autorelease_pool_->Reset();
#endif
    UpdateTitle();
    glfwPollEvents();
    return glfwWindowShouldClose(window_) || exit_request_;
}

AppSize AppBox::GetAppSize() const
{
    return size_;
}

GLFWwindow* AppBox::GetWindow() const
{
    return window_;
}

NativeSurface AppBox::GetNativeSurface() const
{
    return WindowUtils::GetNativeSurface(window_);
}

void AppBox::SwitchFullScreenMode()
{
#if !defined(__APPLE__)
    if (glfwGetWindowMonitor(window_)) {
        glfwSetWindowMonitor(window_, nullptr, window_box_[0], window_box_[1], window_box_[2], window_box_[3], 0);
    } else {
        glfwGetWindowPos(window_, &window_box_[0], &window_box_[1]);
        glfwGetWindowSize(window_, &window_box_[2], &window_box_[3]);
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(window_, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }
#endif
}

// static
void AppBox::OnSizeChanged(GLFWwindow* window, int width, int height)
{
    if (width == 0 && height == 0) {
        return;
    }
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(window));
    self->size_ = WindowUtils::GetSurfaceSize(window);
    if (self->window_listener_) {
        self->window_listener_->OnResize(self->size_.width, self->size_.height);
    }
}

// static
void AppBox::OnKey(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(window));
    self->keys_[key] = action;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        self->exit_request_ = true;
    }

    if (self->keys_[GLFW_KEY_RIGHT_ALT] == GLFW_PRESS && self->keys_[GLFW_KEY_ENTER] == GLFW_PRESS) {
        self->SwitchFullScreenMode();
    }

    if (self->input_listener_) {
        self->input_listener_->OnKey(key, action);
    }
}

// static
void AppBox::OnMouse(GLFWwindow* window, double xpos, double ypos)
{
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(window));
    if (!self->input_listener_) {
        return;
    }
    self->input_listener_->OnMouse(xpos, ypos);
}

// static
void AppBox::OnMouseButton(GLFWwindow* window, int button, int action, int mods)
{
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(window));
    if (!self->input_listener_) {
        return;
    }
    self->input_listener_->OnMouseButton(button, action);
}

// static
void AppBox::OnScroll(GLFWwindow* window, double xoffset, double yoffset)
{
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(window));
    if (!self->input_listener_) {
        return;
    }
    self->input_listener_->OnScroll(xoffset, yoffset);
}

// static
void AppBox::OnInputChar(GLFWwindow* window, uint32_t ch)
{
    AppBox* self = static_cast<AppBox*>(glfwGetWindowUserPointer(window));
    if (!self->input_listener_) {
        return;
    }
    self->input_listener_->OnInputChar(ch);
}
