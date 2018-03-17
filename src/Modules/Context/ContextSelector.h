#pragma once

#include <memory>

#include <Scene/IScene.h>
#include "Context/DX11Context.h"
#include "Context/DX12Context.h"

std::unique_ptr<Context> CreateContext(ApiType type, GLFWwindow* window, int width, int height)
{
    switch (type)
    {
    case ApiType::kDX11:
        return std::make_unique<DX11Context>(window, width, height);
    case ApiType::kDX12:
        return std::make_unique<DX12Context>(window, width, height);
    }
    return nullptr;
}
