#include "Fence/VKFence.h"
#include <Device/VKDevice.h>

VKFence::VKFence(VKDevice& device)
    : m_device(device)
{
    vk::FenceCreateInfo fence_create_info = {};
    fence_create_info.flags = vk::FenceCreateFlagBits::eSignaled;
    m_fence = m_device.GetDevice().createFenceUnique(fence_create_info);
}

void VKFence::Wait()
{
    auto res = m_device.GetDevice().waitForFences(1, &m_fence.get(), VK_TRUE, UINT64_MAX);
    if (res != vk::Result::eSuccess)
        throw std::runtime_error("vkWaitForFences");
}

void VKFence::Reset()
{
    m_device.GetDevice().resetFences(1, &m_fence.get());
}

vk::Fence VKFence::GetFence()
{
    return m_fence.get();
}
