#pragma once
#include "Adapter.h"

#include <vulkan/vulkan.hpp>

class VKInstance;

class VKAdapter : public Adapter {
public:
    VKAdapter(VKInstance& instance, const vk::PhysicalDevice& physical_device);
    const std::string& GetName() const override;
    std::shared_ptr<Device> CreateDevice() override;
    VKInstance& GetInstance();
    vk::PhysicalDevice& GetPhysicalDevice();

private:
    VKInstance& instance_;
    vk::PhysicalDevice physical_device_;
    std::string name_;
};
