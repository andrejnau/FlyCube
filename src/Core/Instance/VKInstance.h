#pragma once
#include "Instance/Instance.h"
#include <vulkan/vulkan.hpp>

class VKInstance : public Instance
{
public:
    VKInstance();
    std::vector<std::shared_ptr<Adapter>> EnumerateAdapters() override;
    vk::Instance& GetInstance();

private:
    vk::DynamicLoader m_dl;
    vk::UniqueInstance m_instance;
    vk::UniqueDebugReportCallbackEXT m_callback;
};
