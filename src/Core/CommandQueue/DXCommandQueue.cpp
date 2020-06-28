#include "CommandQueue/DXCommandQueue.h"
#include <Device/DXDevice.h>
#include <CommandList/DXCommandList.h>
#include <Fence/DXFence.h>
#include <Resource/DXResource.h>
#include <Utilities/DXUtility.h>

DXCommandQueue::DXCommandQueue(DXDevice& device, CommandListType type)
    : CommandQueueBase(device, type)
    , m_device(device)
{
    D3D12_COMMAND_LIST_TYPE dx_type;
    switch (type)
    {
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
        assert(false);
        break;
    }
    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
    queue_desc.Type = dx_type;
    ASSERT_SUCCEEDED(m_device.GetDevice()->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&m_command_queue)));
}

DXCommandQueue::~DXCommandQueue()
{
    OnDestroy();
}

void DXCommandQueue::Wait(const std::shared_ptr<Fence>& fence, uint64_t value)
{
    decltype(auto) dx_fence = fence->As<DXFence>();
    ASSERT_SUCCEEDED(m_command_queue->Wait(dx_fence.GetFence().Get(), value));
}

void DXCommandQueue::Signal(const std::shared_ptr<Fence>& fence, uint64_t value)
{
    decltype(auto) dx_fence = fence->As<DXFence>();
    ASSERT_SUCCEEDED(m_command_queue->Signal(dx_fence.GetFence().Get(), value));
}

void DXCommandQueue::ExecuteCommandListsImpl(const std::vector<std::shared_ptr<CommandList>>& command_lists)
{
    std::vector<ID3D12CommandList*> dx_command_lists;
    for (auto& command_list : command_lists)
    {
        if (!command_list)
            continue;
        decltype(auto) dx_command_list = command_list->As<DXCommandList>();
        dx_command_lists.emplace_back(dx_command_list.GetCommandList().Get());
    }
    if (!dx_command_lists.empty())
        m_command_queue->ExecuteCommandLists(dx_command_lists.size(), dx_command_lists.data());
}

bool DXCommandQueue::AllowCommonStatePromotion(const std::shared_ptr<Resource>& resource, ResourceState state_after)
{
    decltype(auto) dx_resource = resource->As<DXResource>();
    if (dx_resource.desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
        return true;
    if (dx_resource.desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS)
    {
        switch (ConvertSate(state_after))
        {
        case D3D12_RESOURCE_STATE_DEPTH_WRITE:
            return false;
        default:
            return true;
        }
    }
    else
    {
        switch (ConvertSate(state_after))
        {
        case D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
        case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
        case D3D12_RESOURCE_STATE_COPY_DEST:
        case D3D12_RESOURCE_STATE_COPY_SOURCE:
            return true;
        default:
            return false;
        }
    }
    return false;
}

DXDevice& DXCommandQueue::GetDevice()
{
    return m_device;
}

ComPtr<ID3D12CommandQueue> DXCommandQueue::GetQueue()
{
    return m_command_queue;
}