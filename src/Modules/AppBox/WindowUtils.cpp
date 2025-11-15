#include "AppBox/WindowUtils.h"

#include "Utilities/NotReached.h"

#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#import <QuartzCore/CAMetalLayer.h>
#else
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_WAYLAND
#endif

#include <GLFW/glfw3native.h>

namespace {

AppSize GetScreenSize()
{
    auto* monitor_desc = glfwGetVideoMode(glfwGetPrimaryMonitor());
    return AppSize(monitor_desc->width, monitor_desc->height);
}

AppSize GetDefaultWindowsSize(const AppSize& screen_size)
{
    return AppSize(screen_size.width() / 1.5, screen_size.height() / 1.5);
}

} // namespace

namespace WindowUtils {

GLFWwindow* CreateWindowWithDefaultSize(const std::string_view& title)
{
    AppSize screen_size = GetScreenSize();
    AppSize window_size = GetDefaultWindowsSize(screen_size);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(window_size.width(), window_size.height(), title.data(), nullptr, nullptr);

#if defined(__APPLE__)
    NSWindow* ns_window = glfwGetCocoaWindow(window);
    ns_window.contentView.layer = [CAMetalLayer layer];
    ns_window.contentView.wantsLayer = YES;
#endif

    int xpos = (screen_size.width() - window_size.width()) / 2;
    int ypos = (screen_size.height() - window_size.height()) / 2;
    glfwSetWindowPos(window, xpos, ypos);

    return window;
}

AppSize GetSurfaceSize(GLFWwindow* window)
{
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    return AppSize(width, height);
}

NativeSurface GetNativeSurface(GLFWwindow* window)
{
#if defined(_WIN32)
    return Win32Surface{
        .hinstance = GetModuleHandle(nullptr),
        .hwnd = glfwGetWin32Window(window),
    };
#elif defined(__APPLE__)
    NSWindow* ns_window = glfwGetCocoaWindow(window);
    return MetalSurface{
        .ca_metal_layer = (__bridge void*)ns_window.contentView.layer,
    };
#else
    switch (glfwGetPlatform()) {
    case GLFW_PLATFORM_X11:
        return XlibSurface{
            .dpy = glfwGetX11Display(),
            .window = glfwGetX11Window(window),
        };
    case GLFW_PLATFORM_WAYLAND:
        return WaylandSurface{
            .display = glfwGetWaylandDisplay(),
            .surface = glfwGetWaylandWindow(window),
        };
    default:
        NOTREACHED();
    }
#endif
}

} // namespace WindowUtils
