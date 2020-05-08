#pragma once

#include <memory>
#include <Scene/IScene.h>
#include <ApiType/ApiType.h>
#include "Context/Context.h"

std::unique_ptr<Context> CreateContext(ApiType type, GLFWwindow* window);
