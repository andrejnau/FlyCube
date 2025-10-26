#pragma once
#include "AppLoop/AppSize.h"

#include <GLFW/glfw3.h>

#include <string_view>

namespace WindowUtils {

GLFWwindow* CreateWindowWithDefaultSize(const std::string_view& title);
AppSize GetSurfaceSize(GLFWwindow* window);
void* GetNativeWindow(GLFWwindow* window);

} // namespace WindowUtils
