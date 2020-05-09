#include "CommandList.h"
#include <vulkan/vulkan.hpp>

class VKDevice;

class VKCommandList : public CommandList
{
public:
    VKCommandList(VKDevice& device);
    void Open() override;
    void Close() override;
    void Clear(Resource::Ptr iresource, const std::array<float, 4>& color) override;

    vk::CommandBuffer GetCommandList();

private:
    vk::UniqueCommandBuffer m_cmd_buf;
};
