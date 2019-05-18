#include "Context/ContextSelector.h"
#include "Context/DX11Context.h"
#include "Context/DX12Context.h"
#ifdef VULKAN_SUPPORT
#include "Context/VKContext.h"
#endif
#include "Context/GLContext.h"

std::unique_ptr<Context> CreateContext(ApiType type, GLFWwindow* window)
{
    switch (type)
    {
    case ApiType::kDX11:
        return std::make_unique<DX11Context>(window);
    case ApiType::kDX12:
        return std::make_unique<DX12Context>(window);
#ifdef VULKAN_SUPPORT
    case ApiType::kVulkan:
        return std::make_unique<VKContext>(window);
#endif
    case ApiType::kOpenGL:
        return std::make_unique<GLContext>(window);
    }
    assert(false);
    return nullptr;
}
