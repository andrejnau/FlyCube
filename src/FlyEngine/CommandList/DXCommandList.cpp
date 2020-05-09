#include "CommandList/DXCommandList.h"
#include <Device/DXDevice.h>
#include <Utilities/DXUtility.h>
#include <Utilities/FileUtility.h>
#include <dxgi1_6.h>
#include <d3d12.h>

DXCommandList::DXCommandList(DXDevice& device)
    : m_device(device)
{
    ASSERT_SUCCEEDED(device.GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_command_allocator)));
    ASSERT_SUCCEEDED(device.GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_command_allocator.Get(), nullptr, IID_PPV_ARGS(&m_command_list)));
    m_command_list->Close();
    ASSERT_SUCCEEDED(device.GetDevice()->CreateFence(m_fence_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    ++m_fence_value;
    m_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void DXCommandList::Open()
{
    // Schedule a Signal command in the queue.
    const uint64_t current_fence_value = m_fence_value;
    ASSERT_SUCCEEDED(m_device.GetCommandQueue()->Signal(m_fence.Get(), current_fence_value));

    // If the next frame is not ready to be rendered yet, wait until it is ready.
    if (m_fence->GetCompletedValue() < m_fence_value)
    {
        ASSERT_SUCCEEDED(m_fence->SetEventOnCompletion(m_fence_value, m_fence_event));
        WaitForSingleObjectEx(m_fence_event, INFINITE, FALSE);
    }

    // Set the fence value for the next frame.
    m_fence_value = current_fence_value + 1;

    ASSERT_SUCCEEDED(m_command_allocator->Reset());
    ASSERT_SUCCEEDED(m_command_list->Reset(m_command_allocator.Get(), nullptr));
}

void DXCommandList::Close()
{
    m_command_list->Close();
}

void DXCommandList::Clear(Resource::Ptr iresource, const std::array<float, 4>& color)
{
    m_context.command_list->ClearRenderTargetView(ConvertView(view)->GetCpuHandle(), color.data(), 0, nullptr);
}

ComPtr<ID3D12GraphicsCommandList> DXCommandList::GetCommandList()
{
    return m_command_list;
}
