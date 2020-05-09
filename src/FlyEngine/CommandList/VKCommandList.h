#include "CommandList.h"
#include <vulkan/vulkan.hpp>

class VKDevice;

class VKCommandList : public CommandList
{
public:
    VKCommandList(VKDevice& device);
    void Clear(std::shared_ptr<Resource> iresource, const std::array<float, 4>& color) override;

private:
    vk::UniqueCommandBuffer m_cmd_buf;
};
