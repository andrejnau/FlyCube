#include "Instance/Instance.h"

#include "Utilities/NotReached.h"

#if defined(DIRECTX_SUPPORT)
#include "Instance/DXInstance.h"
#endif
#if defined(VULKAN_SUPPORT)
#include "Instance/VKInstance.h"
#endif
#if defined(METAL_SUPPORT)
#include "Instance/MTInstance.h"
#endif

std::shared_ptr<Instance> CreateInstance(ApiType type)
{
    switch (type) {
#if defined(DIRECTX_SUPPORT)
    case ApiType::kDX12:
        return std::make_shared<DXInstance>();
#endif
#if defined(VULKAN_SUPPORT)
    case ApiType::kVulkan:
        return std::make_shared<VKInstance>();
#endif
#if defined(METAL_SUPPORT)
    case ApiType::kMetal:
        return std::make_shared<MTInstance>();
#endif
    default:
        NOTREACHED();
    }
}
