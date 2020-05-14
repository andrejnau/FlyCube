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
    void BindPipeline(const std::shared_ptr<Pipeline>& state) override;
    void BeginRenderPass(const std::shared_ptr<Framebuffer>& framebuffer) override;
    void EndRenderPass() override;
    void Clear(const std::shared_ptr<View>& view, const std::array<float, 4>& color) override;
    void DrawIndexed(uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location) override;
    void ResourceBarrier(const std::shared_ptr<Resource>& resource, ResourceState state) override;
    void SetViewport(float width, float height) override;
    void IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format) override;
    void IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource) override;
    void UpdateSubresource(const std::shared_ptr<Resource>& resource, uint32_t subresource, const void* data, uint32_t row_pitch, uint32_t depth_pitch) override;

    vk::CommandBuffer GetCommandList();

private:
    void ResourceBarrier(const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc, ResourceState state);

    VKDevice& m_device;
    vk::UniqueCommandBuffer m_command_list;
};
