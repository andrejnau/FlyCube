#pragma once
#include "CommandList/CommandList.h"
#include <Utilities/Vulkan.h>

class VKDevice;

class VKCommandList : public CommandList
{
public:
    VKCommandList(VKDevice& device, CommandListType type);
    void Reset() override;
    void Close() override;
    void BindPipeline(const std::shared_ptr<Pipeline>& state) override;
    void BindBindingSet(const std::shared_ptr<BindingSet>& binding_set) override;
    void BeginRenderPass(const std::shared_ptr<RenderPass>& render_pass, const std::shared_ptr<Framebuffer>& framebuffer, const ClearDesc& clear_desc) override;
    void EndRenderPass() override;
    void BeginEvent(const std::string& name) override;
    void EndEvent() override;
    void ClearColor(const std::shared_ptr<View>& view, const glm::vec4& color) override;
    void ClearDepth(const std::shared_ptr<View>& view, float depth) override;
    void DrawIndexed(uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location) override;
    void Dispatch(uint32_t thread_group_count_x, uint32_t thread_group_count_y, uint32_t thread_group_count_z) override;
    void DispatchMesh(uint32_t thread_group_count_x) override;
    void DispatchRays(uint32_t width, uint32_t height, uint32_t depth) override;
    void ResourceBarrier(const std::vector<ResourceBarrierDesc>& barriers) override;
    void UAVResourceBarrier(const std::shared_ptr<Resource>& resource) override;
    void SetViewport(float x, float y, float width, float height) override;
    void SetScissorRect(int32_t left, int32_t top, uint32_t right, uint32_t bottom) override;
    void IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format) override;
    void IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource) override;
    void RSSetShadingRateImage(const std::shared_ptr<View>& view) override;
    void BuildBottomLevelAS(const std::shared_ptr<Resource>& src, const std::shared_ptr<Resource>& dst,
                            const std::shared_ptr<Resource>& scratch, uint64_t scratch_offset, const std::vector<RaytracingGeometryDesc>& descs) override;
    void BuildTopLevelAS(const std::shared_ptr<Resource>& src, const std::shared_ptr<Resource>& dst, const std::shared_ptr<Resource>& scratch, uint64_t scratch_offset,
                         const std::shared_ptr<Resource>& instance_data, uint64_t instance_offset, uint32_t instance_count) override;
    void CopyAccelerationStructure(const std::shared_ptr<Resource>& src, const std::shared_ptr<Resource>& dst, CopyAccelerationStructureMode mode) override;
    void CopyBuffer(const std::shared_ptr<Resource>& src_buffer, const std::shared_ptr<Resource>& dst_buffer,
                    const std::vector<BufferCopyRegion>& regions) override;
    void CopyBufferToTexture(const std::shared_ptr<Resource>& src_buffer, const std::shared_ptr<Resource>& dst_texture,
                             const std::vector<BufferToTextureCopyRegion>& regions) override;
    void CopyTexture(const std::shared_ptr<Resource>& src_texture, const std::shared_ptr<Resource>& dst_texture,
                     const std::vector<TextureCopyRegion>& regions) override;

    vk::CommandBuffer GetCommandList();

private:
    void BuildAccelerationStructure(vk::AccelerationStructureInfoNV& build_info, const vk::Buffer& instance_data, uint64_t instance_offset, const std::shared_ptr<Resource>& src, const std::shared_ptr<Resource>& dst, const std::shared_ptr<Resource>& scratch, uint64_t scratch_offset);

    VKDevice& m_device;
    vk::UniqueCommandBuffer m_command_list;
    std::shared_ptr<Pipeline> m_state;
    std::shared_ptr<BindingSet> m_binding_set;
    bool m_closed = false;
};
