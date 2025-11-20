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
    vk::detail::DynamicLoader dl_;
#endif
    vk::UniqueInstance instance_;
    vk::UniqueDebugUtilsMessengerEXT debug_utils_messenger_;
    bool debug_utils_supported_ = false;
};
