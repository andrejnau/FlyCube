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
    DXCommandList(DXDevice& device);
    void Open() override;
    void Close() override;
    void BindPipeline(const std::shared_ptr<Pipeline>& state) override;
    void BindBindingSet(const std::shared_ptr<BindingSet>& binding_set) override;
    void BeginRenderPass(const std::shared_ptr<Framebuffer>& framebuffer) override;
    void EndRenderPass() override;
    void BeginEvent(const std::string& name) override;
    void EndEvent() override;
    void ClearColor(const std::shared_ptr<View>& view, const std::array<float, 4>& color) override;
    void ClearDepth(const std::shared_ptr<View>& view, float depth) override;
    void DrawIndexed(uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location) override;
    void Dispatch(uint32_t thread_group_count_x, uint32_t thread_group_count_y, uint32_t thread_group_count_z) override;
    void ResourceBarrier(const std::shared_ptr<Resource>& resource, ResourceState state) override;
    void SetViewport(float width, float height) override;
    void IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format) override;
    void IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource) override;
    void UpdateSubresource(const std::shared_ptr<Resource>& resource, uint32_t subresource, const void* data, uint32_t row_pitch, uint32_t depth_pitch) override;

    ComPtr<ID3D12GraphicsCommandList> GetCommandList();

private:
    void ResourceBarrier(DXResource& resource, D3D12_RESOURCE_STATES state);

    DXDevice& m_device;
    ComPtr<ID3D12CommandAllocator> m_command_allocator;
    ComPtr<ID3D12GraphicsCommandList> m_command_list;
    std::vector<ComPtr<ID3D12DescriptorHeap>> m_heaps;
};
