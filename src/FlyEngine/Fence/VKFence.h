#include "Fence.h"
#include <vulkan/vulkan.hpp>

class VKDevice;

class VKFence : public Fence
{
public:
    VKFence(VKDevice& device);
    void Wait() override;
    void Reset() override;

    vk::Fence GetFence();

private:
    VKDevice& m_device;
    vk::UniqueFence m_fence;
};
