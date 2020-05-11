#pragma once
#include "CommandList.h"
#include <Utilities/Vulkan.h>

class VKDevice;

class VKCommandList : public CommandList
{
public:
    VKCommandList(VKDevice& device);
    void Open() override;
    void Close() override;
    void Clear(const std::shared_ptr<View>& view, const std::array<float, 4>& color) override;
    void ResourceBarrier(const std::shared_ptr<Resource>& resource, ResourceState state) override;

    vk::CommandBuffer GetCommandList();

private:
    vk::UniqueCommandBuffer m_cmd_buf;
};
