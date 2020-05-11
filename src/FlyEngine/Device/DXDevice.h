#pragma once
#include "Device/Device.h"
#include "Device/DXViewPool.h"
#include <dxgi.h>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXAdapter;

class DXDevice : public Device
{
public:
    DXDevice(DXAdapter& adapter);
    std::shared_ptr<Swapchain> CreateSwapchain(GLFWwindow* window, uint32_t width, uint32_t height, uint32_t frame_count) override;
    std::shared_ptr<CommandList> CreateCommandList() override;
    std::shared_ptr<Fence> CreateFence() override;
    std::shared_ptr<Semaphore> CreateGPUSemaphore() override;
    std::shared_ptr<View> CreateView(const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc) override;
    void Wait(const std::shared_ptr<Semaphore>& semaphore) override;
    void Signal(const std::shared_ptr<Semaphore>& semaphore) override;
    void ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& command_lists, const std::shared_ptr<Fence>& fence) override;

    DXAdapter& GetAdapter();
    ComPtr<ID3D12Device> GetDevice();
    ComPtr<ID3D12CommandQueue> GetCommandQueue();
    DXViewPool& GetViewPool();

private:
    DXAdapter& m_adapter;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12CommandQueue> m_command_queue;
    DXViewPool m_view_pool;
};
