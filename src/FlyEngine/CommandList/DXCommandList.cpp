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
    ResourceBarrier(dx_resource, dx_state);
}

void DXCommandList::IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format)
{
    DXGI_FORMAT dx_format = static_cast<DXGI_FORMAT>(gli::dx().translate(format).DXGIFormat.DDS);
    DXResource& dx_resource = static_cast<DXResource&>(*resource);
    D3D12_INDEX_BUFFER_VIEW index_buffer_view = {};
    index_buffer_view.Format = dx_format;
    index_buffer_view.SizeInBytes = dx_resource.desc.Width;
    index_buffer_view.BufferLocation = dx_resource.default_res->GetGPUVirtualAddress();
    m_command_list->IASetIndexBuffer(&index_buffer_view);
}

void DXCommandList::IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource)
{
    DXResource& dx_resource = static_cast<DXResource&>(*resource);
    D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view = {};
    vertex_buffer_view.BufferLocation = dx_resource.default_res->GetGPUVirtualAddress();
    vertex_buffer_view.SizeInBytes = dx_resource.desc.Width;
    vertex_buffer_view.StrideInBytes = dx_resource.stride;
    m_command_list->IASetVertexBuffers(slot, 1, &vertex_buffer_view);
}

void DXCommandList::UpdateSubresource(const std::shared_ptr<Resource>& resource, uint32_t subresource, const void* data, uint32_t row_pitch, uint32_t depth_pitch)
{
    DXResource& dx_resource = static_cast<DXResource&>(*resource);

    if (dx_resource.bind_flag & BindFlag::kCbv)
    {
        CD3DX12_RANGE range(0, 0);
        char* cbvGPUAddress = nullptr;
        ASSERT_SUCCEEDED(dx_resource.default_res->Map(0, &range, reinterpret_cast<void**>(&cbvGPUAddress)));
        memcpy(cbvGPUAddress, data, dx_resource .buffer_size);
        dx_resource.default_res->Unmap(0, &range);
        return;
    }

    auto& upload_res = dx_resource.GetUploadResource(subresource);
    if (!upload_res)
    {
        UINT64 buffer_size = GetRequiredIntermediateSize(dx_resource.default_res.Get(), subresource, 1);
        m_device.GetDevice()->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(buffer_size),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&upload_res));
    }

    D3D12_SUBRESOURCE_DATA subresource_data = {};
    subresource_data.pData = data;
    subresource_data.RowPitch = row_pitch;
    subresource_data.SlicePitch = depth_pitch;

    ResourceBarrier(dx_resource, D3D12_RESOURCE_STATE_COPY_DEST);
    UpdateSubresources(m_command_list.Get(), dx_resource.default_res.Get(), upload_res.Get(), 0, subresource, 1, &subresource_data);
    ResourceBarrier(dx_resource, D3D12_RESOURCE_STATE_COMMON);
}

ComPtr<ID3D12GraphicsCommandList> DXCommandList::GetCommandList()
{
    return m_command_list;
}

void DXCommandList::ResourceBarrier(DXResource& resource, D3D12_RESOURCE_STATES state)
{
    if (resource.state == D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE)
        return;
    if (resource.state != state)
        m_command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resource.default_res.Get(), resource.state, state));
    resource.state = state;
}
