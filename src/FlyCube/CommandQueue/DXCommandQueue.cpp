#include "CommandQueue/DXCommandQueue.h"

#include "CommandList/DXCommandList.h"
#include "Device/DXDevice.h"
#include "Fence/DXFence.h"
#include "Resource/DXResource.h"
#include "Utilities/DXUtility.h"
#include "Utilities/NotReached.h"

DXCommandQueue::DXCommandQueue(DXDevice& device, CommandListType type)
    : device_(device)
{
    D3D12_COMMAND_LIST_TYPE dx_type;
    switch (type) {
    case CommandListType::kGraphics:
        dx_type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        break;
    case CommandListType::kCompute:
        dx_type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
        break;
    case CommandListType::kCopy:
        dx_type = D3D12_COMMAND_LIST_TYPE_COPY;
        break;
    default:
        NOTREACHED();
    }
    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
    queue_desc.Type = dx_type;
    CHECK_HRESULT(device_.GetDevice()->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue_)));
}

void DXCommandQueue::Wait(const std::shared_ptr<Fence>& fence, uint64_t value)
{
    decltype(auto) dx_fence = fence->As<DXFence>();
    CHECK_HRESULT(command_queue_->Wait(dx_fence.GetFence().Get(), value));
}

void DXCommandQueue::Signal(const std::shared_ptr<Fence>& fence, uint64_t value)
{
    decltype(auto) dx_fence = fence->As<DXFence>();
    CHECK_HRESULT(command_queue_->Signal(dx_fence.GetFence().Get(), value));
}

void DXCommandQueue::ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& command_lists)
{
    std::vector<ID3D12CommandList*> dx_command_lists;
    for (auto& command_list : command_lists) {
        if (!command_list) {
            continue;
        }
        decltype(auto) dx_command_list = command_list->As<DXCommandList>();
        dx_command_lists.emplace_back(dx_command_list.GetCommandList().Get());
    }
    if (!dx_command_lists.empty()) {
        command_queue_->ExecuteCommandLists(dx_command_lists.size(), dx_command_lists.data());
    }
}

DXDevice& DXCommandQueue::GetDevice()
{
    return device_;
}

ComPtr<ID3D12CommandQueue> DXCommandQueue::GetQueue()
{
    return command_queue_;
}
