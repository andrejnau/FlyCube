#include "CommandList/DXCommandList.h"

#include "BindingSet/DXBindingSet.h"
#include "Device/DXDevice.h"
#include "Framebuffer/DXFramebuffer.h"
#include "Pipeline/DXComputePipeline.h"
#include "Pipeline/DXGraphicsPipeline.h"
#include "Pipeline/DXRayTracingPipeline.h"
#include "QueryHeap/DXRayTracingQueryHeap.h"
#include "RenderPass/DXRenderPass.h"
#include "Resource/DXResource.h"
#include "Utilities/DXUtility.h"
#include "Utilities/SystemUtils.h"
#include "View/DXView.h"

#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include <dxgi1_6.h>
#include <gli/dx.hpp>
#include <nowide/convert.hpp>
#include <pix.h>

#include <stdexcept>

namespace {

D3D12_GPU_VIRTUAL_ADDRESS GetVirtualAddress(const RayTracingShaderTable& table)
{
    if (!table.resource) {
        return 0;
    }
    decltype(auto) dx_resource = table.resource->As<DXResource>();
    return dx_resource.resource->GetGPUVirtualAddress() + table.offset;
}

} // namespace

DXCommandList::DXCommandList(DXDevice& device, CommandListType type)
    : m_device(device)
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
        assert(false);
        break;
    }
    ASSERT_SUCCEEDED(device.GetDevice()->CreateCommandAllocator(dx_type, IID_PPV_ARGS(&m_command_allocator)));
    ASSERT_SUCCEEDED(device.GetDevice()->CreateCommandList(0, dx_type, m_command_allocator.Get(), nullptr,
                                                           IID_PPV_ARGS(&m_command_list)));

    m_command_list.As(&m_command_list4);
    m_command_list.As(&m_command_list5);
    m_command_list.As(&m_command_list6);
}

void DXCommandList::Reset()
{
    Close();
    ASSERT_SUCCEEDED(m_command_allocator->Reset());
    ASSERT_SUCCEEDED(m_command_list->Reset(m_command_allocator.Get(), nullptr));
    m_closed = false;
    m_heaps.clear();
    m_state.reset();
    m_binding_set.reset();
    m_lazy_vertex.clear();
    m_shading_rate_image_view.reset();
}

void DXCommandList::Close()
{
    if (!m_closed) {
        m_command_list->Close();
        m_closed = true;
    }
}

void DXCommandList::BindPipeline(const std::shared_ptr<Pipeline>& state)
{
    if (state == m_state) {
        return;
    }
    m_state = std::static_pointer_cast<DXPipeline>(state);
    m_command_list->SetComputeRootSignature(m_state->GetRootSignature().Get());
    auto type = m_state->GetPipelineType();
    if (type == PipelineType::kGraphics) {
        decltype(auto) dx_state = state->As<DXGraphicsPipeline>();
        m_command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_command_list->SetGraphicsRootSignature(dx_state.GetRootSignature().Get());
        m_command_list->SetPipelineState(dx_state.GetPipeline().Get());
        for (const auto& x : dx_state.GetStrideMap()) {
            auto it = m_lazy_vertex.find(x.first);
            if (it != m_lazy_vertex.end()) {
                IASetVertexBufferImpl(x.first, it->second, x.second);
            } else {
                IASetVertexBufferImpl(x.first, {}, 0);
            }
        }
    } else if (type == PipelineType::kCompute) {
        decltype(auto) dx_state = state->As<DXComputePipeline>();
        m_command_list->SetPipelineState(dx_state.GetPipeline().Get());
    } else if (type == PipelineType::kRayTracing) {
        decltype(auto) dx_state = state->As<DXRayTracingPipeline>();
        m_command_list4->SetPipelineState1(dx_state.GetPipeline().Get());
    }
}

void DXCommandList::BindBindingSet(const std::shared_ptr<BindingSet>& binding_set)
{
    if (binding_set == m_binding_set) {
        return;
    }
    decltype(auto) dx_binding_set = binding_set->As<DXBindingSet>();
    decltype(auto) new_heaps = dx_binding_set.Apply(m_command_list);
    m_heaps.insert(m_heaps.end(), new_heaps.begin(), new_heaps.end());
    m_binding_set = binding_set;
}

void DXCommandList::BeginRenderPass(const std::shared_ptr<RenderPass>& render_pass,
                                    const std::shared_ptr<Framebuffer>& framebuffer,
                                    const ClearDesc& clear_desc)
{
    if (m_device.IsRenderPassesSupported()) {
        BeginRenderPassImpl(render_pass, framebuffer, clear_desc);
    } else {
        OMSetFramebuffer(render_pass, framebuffer, clear_desc);
    }

    decltype(auto) shading_rate_image_view = framebuffer->As<FramebufferBase>().GetDesc().shading_rate_image;
    if (shading_rate_image_view == m_shading_rate_image_view) {
        return;
    }

    if (shading_rate_image_view) {
        decltype(auto) dx_shading_rate_image = shading_rate_image_view->GetResource()->As<DXResource>();
        m_command_list5->RSSetShadingRateImage(dx_shading_rate_image.resource.Get());
    } else {
        m_command_list5->RSSetShadingRateImage(nullptr);
    }
    m_shading_rate_image_view = shading_rate_image_view;
}

D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE Convert(RenderPassLoadOp op)
{
    switch (op) {
    case RenderPassLoadOp::kLoad:
        return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
    case RenderPassLoadOp::kClear:
        return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
    case RenderPassLoadOp::kDontCare:
        return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
    default:
        throw std::runtime_error("Wrong RenderPassLoadOp type");
    }
}

D3D12_RENDER_PASS_ENDING_ACCESS_TYPE Convert(RenderPassStoreOp op)
{
    switch (op) {
    case RenderPassStoreOp::kStore:
        return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
    case RenderPassStoreOp::kDontCare:
        return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
    default:
        throw std::runtime_error("Wrong RenderPassStoreOp type");
    }
}

void DXCommandList::BeginRenderPassImpl(const std::shared_ptr<RenderPass>& render_pass,
                                        const std::shared_ptr<Framebuffer>& framebuffer,
                                        const ClearDesc& clear_desc)
{
    decltype(auto) dx_render_pass = render_pass->As<DXRenderPass>();
    decltype(auto) dx_framebuffer = framebuffer->As<DXFramebuffer>();
    auto& rtvs = dx_framebuffer.GetDesc().colors;
    auto& dsv = dx_framebuffer.GetDesc().depth_stencil;

    auto get_handle = [](const std::shared_ptr<View>& view) {
        if (!view) {
            return D3D12_CPU_DESCRIPTOR_HANDLE{};
        }
        decltype(auto) dx_view = view->As<DXView>();
        return dx_view.GetHandle();
    };

    std::vector<D3D12_RENDER_PASS_RENDER_TARGET_DESC> om_rtv;
    for (uint32_t slot = 0; slot < rtvs.size(); ++slot) {
        auto handle = get_handle(rtvs[slot]);
        if (!handle.ptr) {
            continue;
        }
        D3D12_RENDER_PASS_BEGINNING_ACCESS begin = { Convert(dx_render_pass.GetDesc().colors[slot].load_op), {} };
        if (slot < clear_desc.colors.size()) {
            begin.Clear.ClearValue.Color[0] = clear_desc.colors[slot].r;
            begin.Clear.ClearValue.Color[1] = clear_desc.colors[slot].g;
            begin.Clear.ClearValue.Color[2] = clear_desc.colors[slot].b;
            begin.Clear.ClearValue.Color[3] = clear_desc.colors[slot].a;
        }
        D3D12_RENDER_PASS_ENDING_ACCESS end = { Convert(dx_render_pass.GetDesc().colors[slot].store_op), {} };
        om_rtv.push_back({ handle, begin, end });
    }

    D3D12_RENDER_PASS_DEPTH_STENCIL_DESC om_dsv = {};
    D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* om_dsv_ptr = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE om_dsv_handle = get_handle(dsv);
    if (om_dsv_handle.ptr) {
        D3D12_RENDER_PASS_BEGINNING_ACCESS depth_begin = {
            Convert(dx_render_pass.GetDesc().depth_stencil.depth_load_op), {}
        };
        D3D12_RENDER_PASS_ENDING_ACCESS depth_end = { Convert(dx_render_pass.GetDesc().depth_stencil.depth_store_op),
                                                      {} };
        D3D12_RENDER_PASS_BEGINNING_ACCESS stencil_begin = {
            Convert(dx_render_pass.GetDesc().depth_stencil.stencil_load_op), {}
        };
        D3D12_RENDER_PASS_ENDING_ACCESS stencil_end = {
            Convert(dx_render_pass.GetDesc().depth_stencil.stencil_store_op), {}
        };
        depth_begin.Clear.ClearValue.DepthStencil.Depth = clear_desc.depth;
        stencil_begin.Clear.ClearValue.DepthStencil.Stencil = clear_desc.stencil;
        om_dsv = { om_dsv_handle, depth_begin, stencil_begin, depth_end, stencil_end };
        om_dsv_ptr = &om_dsv;
    }

    m_command_list4->BeginRenderPass(static_cast<uint32_t>(om_rtv.size()), om_rtv.data(), om_dsv_ptr,
                                     D3D12_RENDER_PASS_FLAG_NONE);
}

void DXCommandList::OMSetFramebuffer(const std::shared_ptr<RenderPass>& render_pass,
                                     const std::shared_ptr<Framebuffer>& framebuffer,
                                     const ClearDesc& clear_desc)
{
    decltype(auto) dx_render_pass = render_pass->As<DXRenderPass>();
    decltype(auto) dx_framebuffer = framebuffer->As<DXFramebuffer>();
    auto& rtvs = dx_framebuffer.GetDesc().colors;
    auto& dsv = dx_framebuffer.GetDesc().depth_stencil;

    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> om_rtv(rtvs.size());
    auto get_handle = [](const std::shared_ptr<View>& view) {
        if (!view) {
            return D3D12_CPU_DESCRIPTOR_HANDLE{};
        }
        decltype(auto) dx_view = view->As<DXView>();
        return dx_view.GetHandle();
    };
    for (uint32_t slot = 0; slot < rtvs.size(); ++slot) {
        om_rtv[slot] = get_handle(rtvs[slot]);
        if (dx_render_pass.GetDesc().colors[slot].load_op == RenderPassLoadOp::kClear) {
            m_command_list->ClearRenderTargetView(om_rtv[slot], &clear_desc.colors[slot].x, 0, nullptr);
        }
    }
    while (!om_rtv.empty() && om_rtv.back().ptr == 0) {
        om_rtv.pop_back();
    }
    D3D12_CPU_DESCRIPTOR_HANDLE om_dsv = get_handle(dsv);
    D3D12_CLEAR_FLAGS clear_flags = {};
    if (dx_render_pass.GetDesc().depth_stencil.depth_load_op == RenderPassLoadOp::kClear) {
        clear_flags |= D3D12_CLEAR_FLAG_DEPTH;
    }
    if (dx_render_pass.GetDesc().depth_stencil.stencil_load_op == RenderPassLoadOp::kClear) {
        clear_flags |= D3D12_CLEAR_FLAG_STENCIL;
    }
    if (om_dsv.ptr && clear_flags) {
        m_command_list->ClearDepthStencilView(om_dsv, clear_flags, clear_desc.depth, clear_desc.stencil, 0, nullptr);
    }
    m_command_list->OMSetRenderTargets(static_cast<uint32_t>(om_rtv.size()), om_rtv.data(), FALSE,
                                       om_dsv.ptr ? &om_dsv : nullptr);
}

void DXCommandList::EndRenderPass()
{
    if (!m_device.IsRenderPassesSupported()) {
        return;
    }
    m_command_list4->EndRenderPass();
}

void DXCommandList::BeginEvent(const std::string& name)
{
    if (!m_device.IsUnderGraphicsDebugger()) {
        return;
    }
    PIXBeginEvent(m_command_list.Get(), 0, nowide::widen(name).c_str());
}

void DXCommandList::EndEvent()
{
    if (!m_device.IsUnderGraphicsDebugger()) {
        return;
    }
    PIXEndEvent(m_command_list.Get());
}

void DXCommandList::Draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
{
    m_command_list->DrawInstanced(vertex_count, instance_count, first_vertex, first_instance);
}

void DXCommandList::DrawIndexed(uint32_t index_count,
                                uint32_t instance_count,
                                uint32_t first_index,
                                int32_t vertex_offset,
                                uint32_t first_instance)
{
    m_command_list->DrawIndexedInstanced(index_count, instance_count, first_index, vertex_offset, first_instance);
}

void DXCommandList::ExecuteIndirect(D3D12_INDIRECT_ARGUMENT_TYPE type,
                                    const std::shared_ptr<Resource>& argument_buffer,
                                    uint64_t argument_buffer_offset,
                                    const std::shared_ptr<Resource>& count_buffer,
                                    uint64_t count_buffer_offset,
                                    uint32_t max_draw_count,
                                    uint32_t stride)
{
    decltype(auto) dx_argument_buffer = argument_buffer->As<DXResource>();
    ID3D12Resource* dx_count_buffer = nullptr;
    if (count_buffer) {
        dx_count_buffer = count_buffer->As<DXResource>().resource.Get();
    } else {
        assert(count_buffer_offset == 0);
    }
    m_command_list->ExecuteIndirect(m_device.GetCommandSignature(type, stride), max_draw_count,
                                    dx_argument_buffer.resource.Get(), argument_buffer_offset, dx_count_buffer,
                                    count_buffer_offset);
}

void DXCommandList::DrawIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset)
{
    DrawIndirectCount(argument_buffer, argument_buffer_offset, {}, 0, 1, sizeof(DrawIndirectCommand));
}

void DXCommandList::DrawIndexedIndirect(const std::shared_ptr<Resource>& argument_buffer,
                                        uint64_t argument_buffer_offset)
{
    DrawIndexedIndirectCount(argument_buffer, argument_buffer_offset, {}, 0, 1, sizeof(DrawIndexedIndirectCommand));
}

void DXCommandList::DrawIndirectCount(const std::shared_ptr<Resource>& argument_buffer,
                                      uint64_t argument_buffer_offset,
                                      const std::shared_ptr<Resource>& count_buffer,
                                      uint64_t count_buffer_offset,
                                      uint32_t max_draw_count,
                                      uint32_t stride)
{
    ExecuteIndirect(D3D12_INDIRECT_ARGUMENT_TYPE_DRAW, argument_buffer, argument_buffer_offset, count_buffer,
                    count_buffer_offset, max_draw_count, stride);
}

void DXCommandList::DrawIndexedIndirectCount(const std::shared_ptr<Resource>& argument_buffer,
                                             uint64_t argument_buffer_offset,
                                             const std::shared_ptr<Resource>& count_buffer,
                                             uint64_t count_buffer_offset,
                                             uint32_t max_draw_count,
                                             uint32_t stride)
{
    ExecuteIndirect(D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED, argument_buffer, argument_buffer_offset, count_buffer,
                    count_buffer_offset, max_draw_count, stride);
}

void DXCommandList::Dispatch(uint32_t thread_group_count_x,
                             uint32_t thread_group_count_y,
                             uint32_t thread_group_count_z)
{
    m_command_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z);
}

void DXCommandList::DispatchIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset)
{
    ExecuteIndirect(D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH, argument_buffer, argument_buffer_offset, {}, 0, 1,
                    sizeof(DispatchIndirectCommand));
}

void DXCommandList::DispatchMesh(uint32_t thread_group_count_x)
{
    m_command_list6->DispatchMesh(thread_group_count_x, 1, 1);
}

void DXCommandList::DispatchRays(const RayTracingShaderTables& shader_tables,
                                 uint32_t width,
                                 uint32_t height,
                                 uint32_t depth)
{
    D3D12_DISPATCH_RAYS_DESC dispatch_rays_desc = {};

    dispatch_rays_desc.RayGenerationShaderRecord.StartAddress = GetVirtualAddress(shader_tables.raygen);
    dispatch_rays_desc.RayGenerationShaderRecord.SizeInBytes = shader_tables.raygen.size;

    dispatch_rays_desc.MissShaderTable.StartAddress = GetVirtualAddress(shader_tables.miss);
    dispatch_rays_desc.MissShaderTable.SizeInBytes = shader_tables.miss.size;
    dispatch_rays_desc.MissShaderTable.StrideInBytes = shader_tables.miss.stride;

    dispatch_rays_desc.HitGroupTable.StartAddress = GetVirtualAddress(shader_tables.hit);
    dispatch_rays_desc.HitGroupTable.SizeInBytes = shader_tables.hit.size;
    dispatch_rays_desc.HitGroupTable.StrideInBytes = shader_tables.hit.stride;

    dispatch_rays_desc.CallableShaderTable.StartAddress = GetVirtualAddress(shader_tables.callable);
    dispatch_rays_desc.CallableShaderTable.SizeInBytes = shader_tables.callable.size;
    dispatch_rays_desc.CallableShaderTable.StrideInBytes = shader_tables.callable.stride;

    dispatch_rays_desc.Width = width;
    dispatch_rays_desc.Height = height;
    dispatch_rays_desc.Depth = depth;

    m_command_list4->DispatchRays(&dispatch_rays_desc);
}

void DXCommandList::ResourceBarrier(const std::vector<ResourceBarrierDesc>& barriers)
{
    std::vector<D3D12_RESOURCE_BARRIER> dx_barriers;
    for (const auto& barrier : barriers) {
        if (!barrier.resource) {
            assert(false);
            continue;
        }
        if (barrier.state_before == ResourceState::kRaytracingAccelerationStructure) {
            continue;
        }

        decltype(auto) dx_resource = barrier.resource->As<DXResource>();
        D3D12_RESOURCE_STATES dx_state_before = ConvertState(barrier.state_before);
        D3D12_RESOURCE_STATES dx_state_after = ConvertState(barrier.state_after);
        if (dx_state_before == dx_state_after) {
            continue;
        }

        assert(barrier.base_mip_level + barrier.level_count <= dx_resource.desc.MipLevels);
        assert(barrier.base_array_layer + barrier.layer_count <= dx_resource.desc.DepthOrArraySize);

        if (barrier.base_mip_level == 0 && barrier.level_count == dx_resource.desc.MipLevels &&
            barrier.base_array_layer == 0 && barrier.layer_count == dx_resource.desc.DepthOrArraySize) {
            dx_barriers.emplace_back(
                CD3DX12_RESOURCE_BARRIER::Transition(dx_resource.resource.Get(), dx_state_before, dx_state_after));
        } else {
            for (uint32_t i = barrier.base_mip_level; i < barrier.base_mip_level + barrier.level_count; ++i) {
                for (uint32_t j = barrier.base_array_layer; j < barrier.base_array_layer + barrier.layer_count; ++j) {
                    uint32_t subresource = i + j * dx_resource.desc.MipLevels;
                    dx_barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(
                        dx_resource.resource.Get(), dx_state_before, dx_state_after, subresource));
                }
            }
        }
    }
    if (!dx_barriers.empty()) {
        m_command_list->ResourceBarrier(dx_barriers.size(), dx_barriers.data());
    }
}

void DXCommandList::UAVResourceBarrier(const std::shared_ptr<Resource>& resource)
{
    D3D12_RESOURCE_BARRIER uav_barrier = {};
    uav_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    if (resource) {
        decltype(auto) dx_resource = resource->As<DXResource>();
        uav_barrier.UAV.pResource = dx_resource.resource.Get();
    }
    m_command_list4->ResourceBarrier(1, &uav_barrier);
}

void DXCommandList::SetViewport(float x, float y, float width, float height)
{
    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = x;
    viewport.TopLeftY = y;
    viewport.Width = width;
    viewport.Height = height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    m_command_list->RSSetViewports(1, &viewport);
}

void DXCommandList::SetScissorRect(int32_t left, int32_t top, uint32_t right, uint32_t bottom)
{
    D3D12_RECT rect = { left, top, right, bottom };
    m_command_list->RSSetScissorRects(1, &rect);
}

void DXCommandList::IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format)
{
    DXGI_FORMAT dx_format = static_cast<DXGI_FORMAT>(gli::dx().translate(format).DXGIFormat.DDS);
    decltype(auto) dx_resource = resource->As<DXResource>();
    D3D12_INDEX_BUFFER_VIEW index_buffer_view = {};
    index_buffer_view.Format = dx_format;
    index_buffer_view.SizeInBytes = dx_resource.desc.Width;
    index_buffer_view.BufferLocation = dx_resource.resource->GetGPUVirtualAddress();
    m_command_list->IASetIndexBuffer(&index_buffer_view);
}

void DXCommandList::IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource)
{
    if (m_state && m_state->GetPipelineType() == PipelineType::kGraphics) {
        decltype(auto) dx_state = m_state->As<DXGraphicsPipeline>();
        auto& strides = dx_state.GetStrideMap();
        auto it = strides.find(slot);
        if (it != strides.end()) {
            IASetVertexBufferImpl(slot, resource, it->second);
        } else {
            IASetVertexBufferImpl(slot, {}, 0);
        }
    }
    m_lazy_vertex[slot] = resource;
}

void DXCommandList::IASetVertexBufferImpl(uint32_t slot, const std::shared_ptr<Resource>& resource, uint32_t stride)
{
    D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view = {};
    if (resource) {
        decltype(auto) dx_resource = resource->As<DXResource>();
        vertex_buffer_view.BufferLocation = dx_resource.resource->GetGPUVirtualAddress();
        vertex_buffer_view.SizeInBytes = dx_resource.desc.Width;
        vertex_buffer_view.StrideInBytes = stride;
    }
    m_command_list->IASetVertexBuffers(slot, 1, &vertex_buffer_view);
}

void DXCommandList::RSSetShadingRate(ShadingRate shading_rate, const std::array<ShadingRateCombiner, 2>& combiners)
{
    m_command_list5->RSSetShadingRate(static_cast<D3D12_SHADING_RATE>(shading_rate),
                                      reinterpret_cast<const D3D12_SHADING_RATE_COMBINER*>(combiners.data()));
}

void DXCommandList::BuildAccelerationStructure(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs,
                                               const std::shared_ptr<Resource>& src,
                                               const std::shared_ptr<Resource>& dst,
                                               const std::shared_ptr<Resource>& scratch,
                                               uint64_t scratch_offset)
{
    decltype(auto) dx_dst = dst->As<DXResource>();
    decltype(auto) dx_scratch = scratch->As<DXResource>();

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC acceleration_structure_desc = {};
    acceleration_structure_desc.Inputs = inputs;
    if (src) {
        decltype(auto) dx_src = src->As<DXResource>();
        acceleration_structure_desc.SourceAccelerationStructureData = dx_src.acceleration_structure_handle;
        acceleration_structure_desc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
    }
    acceleration_structure_desc.DestAccelerationStructureData = dx_dst.acceleration_structure_handle;
    acceleration_structure_desc.ScratchAccelerationStructureData =
        dx_scratch.resource->GetGPUVirtualAddress() + scratch_offset;
    m_command_list4->BuildRaytracingAccelerationStructure(&acceleration_structure_desc, 0, nullptr);
}

void DXCommandList::BuildBottomLevelAS(const std::shared_ptr<Resource>& src,
                                       const std::shared_ptr<Resource>& dst,
                                       const std::shared_ptr<Resource>& scratch,
                                       uint64_t scratch_offset,
                                       const std::vector<RaytracingGeometryDesc>& descs,
                                       BuildAccelerationStructureFlags flags)
{
    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometry_descs;
    for (const auto& desc : descs) {
        geometry_descs.emplace_back(FillRaytracingGeometryDesc(desc.vertex, desc.index, desc.flags));
    }
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    inputs.Flags = Convert(flags);
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.NumDescs = geometry_descs.size();
    inputs.pGeometryDescs = geometry_descs.data();
    BuildAccelerationStructure(inputs, src, dst, scratch, scratch_offset);
}

void DXCommandList::BuildTopLevelAS(const std::shared_ptr<Resource>& src,
                                    const std::shared_ptr<Resource>& dst,
                                    const std::shared_ptr<Resource>& scratch,
                                    uint64_t scratch_offset,
                                    const std::shared_ptr<Resource>& instance_data,
                                    uint64_t instance_offset,
                                    uint32_t instance_count,
                                    BuildAccelerationStructureFlags flags)
{
    decltype(auto) dx_instance_data = instance_data->As<DXResource>();
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    inputs.Flags = Convert(flags);
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.NumDescs = instance_count;
    inputs.InstanceDescs = dx_instance_data.resource->GetGPUVirtualAddress() + instance_offset;
    BuildAccelerationStructure(inputs, src, dst, scratch, scratch_offset);
}

void DXCommandList::CopyAccelerationStructure(const std::shared_ptr<Resource>& src,
                                              const std::shared_ptr<Resource>& dst,
                                              CopyAccelerationStructureMode mode)
{
    decltype(auto) dx_src = src->As<DXResource>();
    decltype(auto) dx_dst = dst->As<DXResource>();
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE dx_mode = {};
    switch (mode) {
    case CopyAccelerationStructureMode::kClone:
        dx_mode = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE;
        break;
    case CopyAccelerationStructureMode::kCompact:
        dx_mode = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT;
        break;
    default:
        assert(false);
    }
    m_command_list4->CopyRaytracingAccelerationStructure(dx_dst.acceleration_structure_handle,
                                                         dx_src.acceleration_structure_handle, dx_mode);
}

void DXCommandList::CopyBuffer(const std::shared_ptr<Resource>& src_buffer,
                               const std::shared_ptr<Resource>& dst_buffer,
                               const std::vector<BufferCopyRegion>& regions)
{
    decltype(auto) dx_src_buffer = src_buffer->As<DXResource>();
    decltype(auto) dx_dst_buffer = dst_buffer->As<DXResource>();
    for (const auto& region : regions) {
        m_command_list->CopyBufferRegion(dx_dst_buffer.resource.Get(), region.dst_offset, dx_src_buffer.resource.Get(),
                                         region.src_offset, region.num_bytes);
    }
}

void DXCommandList::CopyBufferToTexture(const std::shared_ptr<Resource>& src_buffer,
                                        const std::shared_ptr<Resource>& dst_texture,
                                        const std::vector<BufferToTextureCopyRegion>& regions)
{
    decltype(auto) dx_src_buffer = src_buffer->As<DXResource>();
    decltype(auto) dx_dst_texture = dst_texture->As<DXResource>();
    auto format = dst_texture->GetFormat();
    DXGI_FORMAT dx_format = static_cast<DXGI_FORMAT>(gli::dx().translate(format).DXGIFormat.DDS);
    for (const auto& region : regions) {
        D3D12_TEXTURE_COPY_LOCATION dst = {};
        dst.pResource = dx_dst_texture.resource.Get();
        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst.SubresourceIndex = region.texture_array_layer * dx_dst_texture.GetLevelCount() + region.texture_mip_level;

        D3D12_TEXTURE_COPY_LOCATION src = {};
        src.pResource = dx_src_buffer.resource.Get();
        src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.PlacedFootprint.Offset = region.buffer_offset;
        src.PlacedFootprint.Footprint.Width = region.texture_extent.width;
        src.PlacedFootprint.Footprint.Height = region.texture_extent.height;
        src.PlacedFootprint.Footprint.Depth = region.texture_extent.depth;
        if (gli::is_compressed(format)) {
            auto extent = gli::block_extent(format);
            src.PlacedFootprint.Footprint.Width = std::max<uint32_t>(extent.x, src.PlacedFootprint.Footprint.Width);
            src.PlacedFootprint.Footprint.Height = std::max<uint32_t>(extent.y, src.PlacedFootprint.Footprint.Height);
            src.PlacedFootprint.Footprint.Depth = std::max<uint32_t>(extent.z, src.PlacedFootprint.Footprint.Depth);
        }
        src.PlacedFootprint.Footprint.RowPitch = region.buffer_row_pitch;
        src.PlacedFootprint.Footprint.Format = dx_format;

        m_command_list->CopyTextureRegion(&dst, region.texture_offset.x, region.texture_offset.y,
                                          region.texture_offset.z, &src, nullptr);
    }
}

void DXCommandList::CopyTexture(const std::shared_ptr<Resource>& src_texture,
                                const std::shared_ptr<Resource>& dst_texture,
                                const std::vector<TextureCopyRegion>& regions)
{
    decltype(auto) dx_src_texture = src_texture->As<DXResource>();
    decltype(auto) dx_dst_texture = dst_texture->As<DXResource>();
    for (const auto& region : regions) {
        D3D12_TEXTURE_COPY_LOCATION dst = {};
        dst.pResource = dx_dst_texture.resource.Get();
        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst.SubresourceIndex = region.dst_array_layer * dx_dst_texture.GetLevelCount() + region.dst_mip_level;

        D3D12_TEXTURE_COPY_LOCATION src = {};
        src.pResource = dx_src_texture.resource.Get();
        src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        src.SubresourceIndex = region.src_array_layer * dx_src_texture.GetLevelCount() + region.src_mip_level;

        D3D12_BOX src_box = {};
        src_box.left = region.src_offset.x;
        src_box.top = region.src_offset.y;
        src_box.front = region.src_offset.z;
        src_box.right = region.src_offset.x + region.extent.width;
        src_box.bottom = region.src_offset.y + region.extent.height;
        src_box.back = region.src_offset.z + region.extent.depth;

        m_command_list->CopyTextureRegion(&dst, region.dst_offset.x, region.dst_offset.y, region.dst_offset.z, &src,
                                          &src_box);
    }
}

void DXCommandList::WriteAccelerationStructuresProperties(
    const std::vector<std::shared_ptr<Resource>>& acceleration_structures,
    const std::shared_ptr<QueryHeap>& query_heap,
    uint32_t first_query)
{
    if (query_heap->GetType() != QueryHeapType::kAccelerationStructureCompactedSize) {
        assert(false);
        return;
    }
    decltype(auto) dx_query_heap = query_heap->As<DXRayTracingQueryHeap>();
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC desc = {};
    desc.InfoType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE;
    desc.DestBuffer = dx_query_heap.GetResource()->GetGPUVirtualAddress() + first_query * sizeof(uint64_t);
    std::vector<D3D12_GPU_VIRTUAL_ADDRESS> dx_acceleration_structures;
    dx_acceleration_structures.reserve(acceleration_structures.size());
    for (const auto& acceleration_structure : acceleration_structures) {
        dx_acceleration_structures.emplace_back(acceleration_structure->As<DXResource>().acceleration_structure_handle);
    }
    m_command_list4->EmitRaytracingAccelerationStructurePostbuildInfo(&desc, dx_acceleration_structures.size(),
                                                                      dx_acceleration_structures.data());
}

void DXCommandList::ResolveQueryData(const std::shared_ptr<QueryHeap>& query_heap,
                                     uint32_t first_query,
                                     uint32_t query_count,
                                     const std::shared_ptr<Resource>& dst_buffer,
                                     uint64_t dst_offset)
{
    if (query_heap->GetType() != QueryHeapType::kAccelerationStructureCompactedSize) {
        assert(false);
        return;
    }

    decltype(auto) dx_query_heap = query_heap->As<DXRayTracingQueryHeap>();
    decltype(auto) dx_dst_buffer = dst_buffer->As<DXResource>();
    m_command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dx_query_heap.GetResource().Get(),
                                                                             D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                                             D3D12_RESOURCE_STATE_COPY_SOURCE, 0));
    m_command_list->CopyBufferRegion(dx_dst_buffer.resource.Get(), dst_offset, dx_query_heap.GetResource().Get(),
                                     first_query * sizeof(uint64_t), query_count * sizeof(uint64_t));
    m_command_list->ResourceBarrier(
        1, &CD3DX12_RESOURCE_BARRIER::Transition(dx_query_heap.GetResource().Get(), D3D12_RESOURCE_STATE_COPY_SOURCE,
                                                 D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0));
}

ComPtr<ID3D12GraphicsCommandList> DXCommandList::GetCommandList()
{
    return m_command_list;
}
