#pragma once
#include "CommandList/CommandList.h"
#include <dxgi.h>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXDevice;
class DXResource;

class DXCommandList : public CommandList
{
public:
    DXCommandList(DXDevice& device, CommandListType type);
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
    void DispatchRays(uint32_t width, uint32_t height, uint32_t depth) override;
    void ResourceBarrier(const std::vector<ResourceBarrierDesc>& barriers) override;
    void SetViewport(float x, float y, float width, float height) override;
    void SetScissorRect(int32_t left, int32_t top, uint32_t right, uint32_t bottom) override;
    void IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format) override;
    void IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource) override;
    void RSSetShadingRate(ShadingRate shading_rate, const std::array<ShadingRateCombiner, 2>& combiners) override;
    void RSSetShadingRateImage(const std::shared_ptr<Resource>& resource) override;
    void BuildBottomLevelAS(const std::shared_ptr<Resource>& result, const std::shared_ptr<Resource>& scratch, const std::vector<RaytracingGeometryDesc>& descs) override;
    void BuildTopLevelAS(const std::shared_ptr<Resource>& result, const std::shared_ptr<Resource>& scratch, const std::shared_ptr<Resource>& instance_data, uint32_t instance_count) override;
    void CopyAccelerationStructure(const std::shared_ptr<Resource>& src, const std::shared_ptr<Resource>& dst, CopyAccelerationStructureMode mode) override;
    void CopyBuffer(const std::shared_ptr<Resource>& src_buffer, const std::shared_ptr<Resource>& dst_buffer,
                    const std::vector<BufferCopyRegion>& regions) override;
    void CopyBufferToTexture(const std::shared_ptr<Resource>& src_buffer, const std::shared_ptr<Resource>& dst_texture,
                             const std::vector<BufferToTextureCopyRegion>& regions) override;
    void CopyTexture(const std::shared_ptr<Resource>& src_texture, const std::shared_ptr<Resource>& dst_texture,
                     const std::vector<TextureCopyRegion>& regions) override;

    ComPtr<ID3D12GraphicsCommandList> GetCommandList();

private:
    void BeginRenderPassImpl(const std::shared_ptr<RenderPass>& render_pass, const std::shared_ptr<Framebuffer>& framebuffer, const ClearDesc& clear_desc);
    void OMSetFramebuffer(const std::shared_ptr<RenderPass>& render_pass, const std::shared_ptr<Framebuffer>& framebuffer, const ClearDesc& clear_desc);
    void IASetVertexBufferImpl(uint32_t slot, const std::shared_ptr<Resource>& resource, uint32_t stride);
    void BuildAccelerationStructure(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs, const std::shared_ptr<Resource>& result, const std::shared_ptr<Resource>& scratch);

    DXDevice& m_device;
    ComPtr<ID3D12CommandAllocator> m_command_allocator;
    ComPtr<ID3D12GraphicsCommandList> m_command_list;
    std::vector<ComPtr<ID3D12DescriptorHeap>> m_heaps;
    std::shared_ptr<Pipeline> m_state;
    std::shared_ptr<BindingSet> m_binding_set;
    std::map<uint32_t, std::shared_ptr<Resource>> m_lazy_vertex;
    bool m_use_render_passes = true;
    ComPtr<ID3D12GraphicsCommandList4> m_command_list4;
    ComPtr<ID3D12GraphicsCommandList5> m_command_list5;
    bool m_closed = false;
};
