#include "CommandList/DXCommandList.h"
#include <Device/DXDevice.h>
#include <Resource/DX12Resource.h>
#include <Utilities/DXUtility.h>
#include <Utilities/FileUtility.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3dx12.h>

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

void DXCommandList::Clear(Resource::Ptr resource, const std::array<float, 4>& color)
{
    DX12Resource& dx_resource = (DX12Resource&)*resource;

    auto get_handle = [&]()
    {
        static std::map<void*, CD3DX12_CPU_DESCRIPTOR_HANDLE> q;
        static std::map<void*, ComPtr<ID3D12DescriptorHeap>> w;
        if (q.count(&dx_resource))
            return q[&dx_resource];

        D3D12_RESOURCE_DESC desc = dx_resource.default_res->GetDesc();
        D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
        rtv_desc.Format = desc.Format;
        rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtv_desc.Texture2D.MipSlice = 0;

        ComPtr<ID3D12DescriptorHeap> m_descriptor_heap;
        D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
        heap_desc.NumDescriptors = 1;
        heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

        ASSERT_SUCCEEDED(m_device.GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&m_descriptor_heap)));
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(m_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
        m_device.GetDevice()->CreateRenderTargetView(dx_resource.default_res.Get(), &rtv_desc, rtv_handle);
        w[&dx_resource] = m_descriptor_heap;
        return q[&dx_resource] = rtv_handle;
    };

    m_command_list->ClearRenderTargetView(get_handle(), color.data(), 0, nullptr);
}

void DXCommandList::ResourceBarrier(Resource::Ptr resource, ResourceState state)
{
    DX12Resource& dx_resource = (DX12Resource&)*resource;

    D3D12_RESOURCE_STATES dx_state = {};

    switch (state)
    {
    case ResourceState::kCommon:
        dx_state = D3D12_RESOURCE_STATE_COMMON;
        break;
    case ResourceState::kPresent:
        dx_state = D3D12_RESOURCE_STATE_PRESENT;
        break;
    case ResourceState::kClear:
    case ResourceState::kRenderTarget:
        dx_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
        break;
    case ResourceState::kUnorderedAccess:
        dx_state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        break;
    }

    if (dx_resource.state == D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE)
        return;
    if (dx_resource.state != dx_state)
        m_command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dx_resource.default_res.Get(), dx_resource.state, dx_state));
    dx_resource.state = dx_state;
}

ComPtr<ID3D12GraphicsCommandList> DXCommandList::GetCommandList()
{
    return m_command_list;
}
