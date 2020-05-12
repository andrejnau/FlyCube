#include "Instance/Instance.h"
#ifdef DIRECTX_SUPPORT
#include "Instance/DXInstance.h"
#endif
#ifdef VULKAN_SUPPORT
#include "Instance/VKInstance.h"
#endif
#include <cassert>

std::shared_ptr<Instance> CreateInstance(ApiType type)
{
    switch (type)
    {
#ifdef DIRECTX_SUPPORT
    case ApiType::kDX12:
        return std::make_unique<DXInstance>();
#endif
#ifdef VULKAN_SUPPORT
    case ApiType::kVulkan:
        return std::make_unique<VKInstance>();
#endif
    }
    assert(false);
    return nullptr;
}
