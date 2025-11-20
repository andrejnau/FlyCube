#pragma once
#include "CommandQueue/CommandQueue.h"

#if defined(_WIN32)
#include <wrl.h>
#else
#include <wsl/wrladapter.h>
#endif

#include <directx/d3d12.h>

using Microsoft::WRL::ComPtr;

class DXDevice;

class DXCommandQueue : public CommandQueue {
public:
    DXCommandQueue(DXDevice& device, CommandListType type);
    void Wait(const std::shared_ptr<Fence>& fence, uint64_t value) override;
    void Signal(const std::shared_ptr<Fence>& fence, uint64_t value) override;
    void ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& command_lists) override;

    DXDevice& GetDevice();
    ComPtr<ID3D12CommandQueue> GetQueue();

private:
    DXDevice& device_;
    ComPtr<ID3D12CommandQueue> command_queue_;
};
