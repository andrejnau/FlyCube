#include "Instance/Instance.h"
#ifdef DIRECTX_SUPPORT
#include "Instance/DXInstance.h"
#endif
#ifdef VULKAN_SUPPORT
#include "Instance/VKInstance.h"
#endif
#ifdef METAL_SUPPORT
#include "Instance/MTInstance.h"
#endif
#include <cassert>

std::shared_ptr<Instance> CreateInstance(ApiType type)
{
    switch (type) {
#ifdef DIRECTX_SUPPORT
    case ApiType::kDX12:
        return std::make_shared<DXInstance>();
#endif
#ifdef VULKAN_SUPPORT
    case ApiType::kVulkan:
        return std::make_shared<VKInstance>();
#endif
#ifdef METAL_SUPPORT
    case ApiType::kMetal:
        return std::make_shared<MTInstance>();
#endif
    default:
        assert(false);
        return nullptr;
    }
}
