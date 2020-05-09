#include "Semaphore.h"
#include <vulkan/vulkan.hpp>

class VKDevice;

class VKSemaphore : public Semaphore
{
public:
    VKSemaphore(VKDevice& device);

    const vk::Semaphore& GetSemaphore() const;

private:
    VKDevice& m_device;
    vk::UniqueSemaphore m_semaphore;
};
