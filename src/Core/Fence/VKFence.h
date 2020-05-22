#include "Fence.h"
#include <Utilities/Vulkan.h>

class VKDevice;

class VKFence : public Fence
{
public:
    VKFence(VKDevice& device);
    void WaitAndReset() override;

    vk::Fence GetFence();

private:
    VKDevice& m_device;
    vk::UniqueFence m_fence;
};
