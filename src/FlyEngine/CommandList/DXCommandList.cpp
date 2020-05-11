#include "CommandList/DXCommandList.h"
#include <Device/DXDevice.h>
#include <Resource/DXResource.h>
#include <View/DXView.h>
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
}

void DXCommandList::Open()
{
    ASSERT_SUCCEEDED(m_command_allocator->Reset());
    ASSERT_SUCCEEDED(m_command_list->Reset(m_command_allocator.Get(), nullptr));
}

void DXCommandList::Close()
{
    m_command_list->Close();
}

void DXCommandList::Clear(const std::shared_ptr<View>& view, const std::array<float, 4>& color)
{
    if (!view)
        return;
    DXView& dx_view = static_cast<DXView&>(*view);
    m_command_list->ClearRenderTargetView(dx_view.GetHandle(), color.data(), 0, nullptr);
}

void DXCommandList::ResourceBarrier(const std::shared_ptr<Resource>& resource, ResourceState state)
{
    DXResource& dx_resource = static_cast<DXResource&>(*resource);

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
