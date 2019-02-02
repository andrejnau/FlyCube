#include "Context/ContextSelector.h"
#include "Context/DX11Context.h"
#include "Context/DX12Context.h"
#include "Context/VKContext.h"
#include "Context/GLContext.h"

std::unique_ptr<Context> CreateContext(ApiType type, GLFWwindow* window)
{
    switch (type)
    {
    case ApiType::kDX11:
        return std::make_unique<DX11Context>(window);
    case ApiType::kDX12:
        return std::make_unique<DX12Context>(window);
    case ApiType::kVulkan:
        return std::make_unique<VKContext>(window);
    case ApiType::kOpenGL:
        return std::make_unique<GLContext>(window);
    }
    return nullptr;
}
