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
    void Clear(Resource::Ptr resource, const std::array<float, 4>& color) override;
    void ResourceBarrier(Resource::Ptr resource, ResourceState state) override;

    ComPtr<ID3D12GraphicsCommandList> GetCommandList();

private:
    DXDevice& m_device;
    ComPtr<ID3D12CommandAllocator> m_command_allocator;
    ComPtr<ID3D12GraphicsCommandList> m_command_list;
};
