#pragma once
#include "Instance/Instance.h"

#include <vulkan/vulkan.hpp>

class VKInstance : public Instance {
public:
    VKInstance();
    std::vector<std::shared_ptr<Adapter>> EnumerateAdapters() override;
    vk::Instance& GetInstance();

    bool IsDebugUtilsSupported() const;

private:
#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
    vk::detail::DynamicLoader m_dl;
#endif
    vk::UniqueInstance m_instance;
    vk::UniqueDebugUtilsMessengerEXT m_debug_utils_messenger;
    bool m_debug_utils_supported = false;
};
