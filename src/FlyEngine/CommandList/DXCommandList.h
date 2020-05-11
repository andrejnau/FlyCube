#pragma once
#include "CommandList/CommandList.h"
#include <dxgi.h>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXDevice;

class DXCommandList : public CommandList
{
public:
    DXCommandList(DXDevice& device);
    void Open() override;
    void Close() override;
    void Clear(const std::shared_ptr<View>& view, const std::array<float, 4>& color) override;
    void ResourceBarrier(const std::shared_ptr<Resource>& resource, ResourceState state) override;
    void IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format) override;
    void IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource) override;

    ComPtr<ID3D12GraphicsCommandList> GetCommandList();

private:
    DXDevice& m_device;
    ComPtr<ID3D12CommandAllocator> m_command_allocator;
    ComPtr<ID3D12GraphicsCommandList> m_command_list;
};
