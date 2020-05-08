#include "Device.h"
#include <vulkan/vulkan.hpp>

class VKDevice : public Device
{
public:
    VKDevice(const vk::PhysicalDevice& physical_device);
    vk::Device GetDevice();

private:
    vk::UniqueDevice m_device;
};
