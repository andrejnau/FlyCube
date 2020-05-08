#include "Adapter.h"
#include <vulkan/vulkan.hpp>

class VKAdapter : public Adapter
{
public:
    VKAdapter(const vk::PhysicalDevice& physical_device);
    const std::string& GetName() const override;
    std::unique_ptr<Device> CreateDevice() override;
    vk::PhysicalDevice& GetPhysicalDevice();

private:
    vk::PhysicalDevice m_physical_device;
    std::string m_name;
};
