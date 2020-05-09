#include "Instance/Instance.h"
#include <vulkan/vulkan.hpp>

class VKInstance : public Instance
{
public:
    VKInstance();
    std::vector<std::unique_ptr<Adapter>> EnumerateAdapters() override;
    vk::Instance& GetInstance();

private:
    vk::UniqueInstance m_instance;
};
