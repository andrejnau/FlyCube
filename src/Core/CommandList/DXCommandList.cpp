#include "CommandList/DXCommandList.h"
#include <Device/DXDevice.h>
#include <Resource/DXResource.h>
#include <View/DXView.h>
#include <Utilities/DXUtility.h>
#include <Utilities/FileUtility.h>
#include <Pipeline/DXGraphicsPipeline.h>
#include <Pipeline/DXComputePipeline.h>
#include <Pipeline/DXRayTracingPipeline.h>
#include <Framebuffer/DXFramebuffer.h>
#include <BindingSet/DXBindingSet.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <gli/dx.hpp>
#include <pix.h>

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
    m_heaps.clear();
    m_state.reset();
}

void DXCommandList::Close()
{
    m_command_list->Close();
}

void DXCommandList::BindPipeline(const std::shared_ptr<Pipeline>& state)
{
    auto type = state->GetPipelineType();
    if (type == PipelineType::kGraphics)
    {
        decltype(auto) dx_state = state->As<DXGraphicsPipeline>();
        m_command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_command_list->SetGraphicsRootSignature(dx_state.GetRootSignature().Get());
        m_command_list->SetPipelineState(dx_state.GetPipeline().Get());
    }
    else if (type == PipelineType::kCompute)
    {
        decltype(auto) dx_state = state->As<DXComputePipeline>();
        m_command_list->SetComputeRootSignature(dx_state.GetRootSignature().Get());
        m_command_list->SetPipelineState(dx_state.GetPipeline().Get());
    }
    else if (type == PipelineType::kRayTracing)
    {
        decltype(auto) dx_state = state->As<DXRayTracingPipeline>();
        ComPtr<ID3D12GraphicsCommandList4> command_list4;
        m_command_list.As(&command_list4);
        m_command_list->SetComputeRootSignature(dx_state.GetRootSignature().Get());
        command_list4->SetPipelineState1(dx_state.GetPipeline().Get());
    }
    m_state = state;
}

void DXCommandList::BindBindingSet(const std::shared_ptr<BindingSet>& binding_set)
{
    decltype(auto) dx_binding_set = binding_set->As<DXBindingSet>();
    decltype(auto) new_heaps = dx_binding_set.Apply(m_command_list);
    m_heaps.insert(m_heaps.end(), new_heaps.begin(), new_heaps.end());
}

void DXCommandList::BeginRenderPass(const std::shared_ptr<Framebuffer>& framebuffer)
{
    decltype(auto) dx_framebuffer = framebuffer->As<DXFramebuffer>();
    auto& rtvs = dx_framebuffer.GetRtvs();
    auto& dsv = dx_framebuffer.GetDsv();

    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> om_rtv(rtvs.size());
    auto get_handle = [](const std::shared_ptr<View>& view)
    {
        if (!view)
            return D3D12_CPU_DESCRIPTOR_HANDLE{};
        decltype(auto) dx_view = view->As<DXView>();
        return dx_view.GetHandle();
    };
    for (uint32_t slot = 0; slot < rtvs.size(); ++slot)
    {
        om_rtv[slot] = get_handle(rtvs[slot]);
    }
    D3D12_CPU_DESCRIPTOR_HANDLE om_dsv = get_handle(dsv);
    m_command_list->OMSetRenderTargets(static_cast<uint32_t>(om_rtv.size()), om_rtv.data(), FALSE, om_dsv.ptr ? &om_dsv : nullptr);
}

void DXCommandList::EndRenderPass()
{
}

void DXCommandList::BeginEvent(const std::string& name)
{
    std::wstring wname = utf8_to_wstring(name);
    PIXBeginEvent(m_command_list.Get(), 0, wname.c_str());
}

void DXCommandList::EndEvent()
{
    PIXEndEvent(m_command_list.Get());
}

void DXCommandList::ClearColor(const std::shared_ptr<View>& view, const std::array<float, 4>& color)
{
    if (!view)
        return;
    decltype(auto) dx_view = view->As<DXView>();
    m_command_list->ClearRenderTargetView(dx_view.GetHandle(), color.data(), 0, nullptr);
}

void DXCommandList::ClearDepth(const std::shared_ptr<View>& view, float depth)
{
    if (!view)
        return;
    decltype(auto) dx_view = view->As<DXView>();
    m_command_list->ClearDepthStencilView(dx_view.GetHandle(), D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void DXCommandList::DrawIndexed(uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location)
{
    m_command_list->DrawIndexedInstanced(index_count, 1, start_index_location, base_vertex_location, 0);
}

void DXCommandList::Dispatch(uint32_t thread_group_count_x, uint32_t thread_group_count_y, uint32_t thread_group_count_z)
{
    m_command_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z);
}

void DXCommandList::DispatchRays(uint32_t width, uint32_t height, uint32_t depth)
{
    decltype(auto) dx_state = m_state->As<DXRayTracingPipeline>();
    ComPtr<ID3D12GraphicsCommandList4> command_list4;
    m_command_list.As(&command_list4);
    D3D12_DISPATCH_RAYS_DESC raytrace_desc = dx_state.GetDispatchRaysDesc();
    raytrace_desc.Width = width;
    raytrace_desc.Height = height;
    raytrace_desc.Depth = depth;
    command_list4->DispatchRays(&raytrace_desc);
}

void DXCommandList::ResourceBarrier(const std::shared_ptr<Resource>& resource, ResourceState state)
{
    if (!resource)
        return;

    decltype(auto) dx_resource = resource->As<DXResource>();
    D3D12_RESOURCE_STATES dx_state = {};
    switch (state)
    {
    case ResourceState::kCommon:
        dx_state = D3D12_RESOURCE_STATE_COMMON;
        break;
    case ResourceState::kPresent:
        dx_state = D3D12_RESOURCE_STATE_PRESENT;
        break;
    case ResourceState::kClearColor:
    case ResourceState::kRenderTarget:
        dx_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
        break;
    case ResourceState::kClearDepth:
    case ResourceState::kDepthTarget:
        dx_state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        break;
    case ResourceState::kUnorderedAccess:
        dx_state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        break;
    case ResourceState::kPixelShaderResource:
        dx_state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        break;
    case ResourceState::kNonPixelShaderResource:
        dx_state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        break;
    }
    ResourceBarrier(dx_resource, dx_state);
}

void DXCommandList::SetViewport(float width, float height)
{
    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = (float)width;
    viewport.Height = (float)height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    m_command_list->RSSetViewports(1, &viewport);

    D3D12_RECT rect = { 0, 0, width, height };
    m_command_list->RSSetScissorRects(1, &rect);
}

void DXCommandList::IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format)
{
    DXGI_FORMAT dx_format = static_cast<DXGI_FORMAT>(gli::dx().translate(format).DXGIFormat.DDS);
    decltype(auto) dx_resource = resource->As<DXResource>();
    D3D12_INDEX_BUFFER_VIEW index_buffer_view = {};
    index_buffer_view.Format = dx_format;
    index_buffer_view.SizeInBytes = dx_resource.desc.Width;
    index_buffer_view.BufferLocation = dx_resource.default_res->GetGPUVirtualAddress();
    m_command_list->IASetIndexBuffer(&index_buffer_view);
}

void DXCommandList::IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource)
{
    decltype(auto) dx_resource = resource->As<DXResource>();
    D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view = {};
    vertex_buffer_view.BufferLocation = dx_resource.default_res->GetGPUVirtualAddress();
    vertex_buffer_view.SizeInBytes = dx_resource.desc.Width;
    vertex_buffer_view.StrideInBytes = dx_resource.stride;
    m_command_list->IASetVertexBuffers(slot, 1, &vertex_buffer_view);
}

void DXCommandList::UpdateSubresource(const std::shared_ptr<Resource>& resource, uint32_t subresource, const void* data, uint32_t row_pitch, uint32_t depth_pitch)
{
    decltype(auto) dx_resource = resource->As<DXResource>();

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