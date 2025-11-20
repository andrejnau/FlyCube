#include "CommandList/DXCommandList.h"

#include "BindingSet/DXBindingSet.h"
#include "Device/DXDevice.h"
#include "Pipeline/DXComputePipeline.h"
#include "Pipeline/DXGraphicsPipeline.h"
#include "Pipeline/DXRayTracingPipeline.h"
#include "QueryHeap/DXRayTracingQueryHeap.h"
#include "Resource/DXResource.h"
#include "Utilities/DXUtility.h"
#include "Utilities/NotReached.h"
#include "Utilities/SystemUtils.h"
#include "View/DXView.h"

#include <directx/d3dx12.h>
#include <gli/dx.hpp>
#include <nowide/convert.hpp>

#if defined(_WIN32)
#include <pix.h>
#endif

namespace {

D3D12_GPU_VIRTUAL_ADDRESS GetVirtualAddress(const RayTracingShaderTable& table)
{
    if (!table.resource) {
        return 0;
    }
    decltype(auto) dx_resource = table.resource->As<DXResource>();
    return dx_resource.GetResource()->GetGPUVirtualAddress() + table.offset;
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
        NOTREACHED();
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
        NOTREACHED();
    }
}

} // namespace

DXCommandList::DXCommandList(DXDevice& device, CommandListType type)
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
    CHECK_HRESULT(device.GetDevice()->CreateCommandAllocator(dx_type, IID_PPV_ARGS(&command_allocator_)));
    CHECK_HRESULT(device.GetDevice()->CreateCommandList(0, dx_type, command_allocator_.Get(), nullptr,
                                                        IID_PPV_ARGS(&command_list_)));

    command_list_.As(&command_list4_);
    command_list_.As(&command_list5_);
    command_list_.As(&command_list6_);
}

void DXCommandList::Reset()
{
    Close();
    CHECK_HRESULT(command_allocator_->Reset());
    CHECK_HRESULT(command_list_->Reset(command_allocator_.Get(), nullptr));
    closed_ = false;
    heaps_.clear();
    state_.reset();
    binding_set_.reset();
    lazy_vertex_.clear();
    shading_rate_image_view_.reset();
}

void DXCommandList::Close()
{
    if (!closed_) {
        command_list_->Close();
        closed_ = true;
    }
}

void DXCommandList::BindPipeline(const std::shared_ptr<Pipeline>& state)
{
    if (state == state_) {
        return;
    }
    state_ = std::static_pointer_cast<DXPipeline>(state);
    command_list_->SetComputeRootSignature(state_->GetRootSignature().Get());
    auto type = state_->GetPipelineType();
    if (type == PipelineType::kGraphics) {
        decltype(auto) dx_state = state->As<DXGraphicsPipeline>();
        command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        command_list_->SetGraphicsRootSignature(dx_state.GetRootSignature().Get());
        command_list_->SetPipelineState(dx_state.GetPipeline().Get());
        for (const auto& [slot, stride] : dx_state.GetStrideMap()) {
            auto it = lazy_vertex_.find(slot);
            if (it != lazy_vertex_.end()) {
                const auto& [resource, offset] = it->second;
                IASetVertexBufferImpl(slot, resource, offset, stride);
            } else {
                IASetVertexBufferImpl(slot, nullptr, 0, 0);
            }
        }
    } else if (type == PipelineType::kCompute) {
        decltype(auto) dx_state = state->As<DXComputePipeline>();
        command_list_->SetPipelineState(dx_state.GetPipeline().Get());
    } else if (type == PipelineType::kRayTracing) {
        decltype(auto) dx_state = state->As<DXRayTracingPipeline>();
        command_list4_->SetPipelineState1(dx_state.GetPipeline().Get());
    }
}

void DXCommandList::BindBindingSet(const std::shared_ptr<BindingSet>& binding_set)
{
    if (binding_set == binding_set_) {
        return;
    }
    decltype(auto) dx_binding_set = binding_set->As<DXBindingSet>();
    decltype(auto) new_heaps = dx_binding_set.Apply(command_list_);
    heaps_.insert(heaps_.end(), new_heaps.begin(), new_heaps.end());
    binding_set_ = binding_set;
}

void DXCommandList::BeginRenderPass(const RenderPassDesc& render_pass_desc)
{
    auto get_handle = [](const std::shared_ptr<View>& view) {
        if (!view) {
            return D3D12_CPU_DESCRIPTOR_HANDLE{};
        }
        decltype(auto) dx_view = view->As<DXView>();
        return dx_view.GetHandle();
    };

    std::vector<D3D12_RENDER_PASS_RENDER_TARGET_DESC> om_rtv;
    for (size_t i = 0; i < render_pass_desc.colors.size(); ++i) {
        auto handle = get_handle(render_pass_desc.colors[i].view);
        if (!handle.ptr) {
            continue;
        }
        D3D12_RENDER_PASS_BEGINNING_ACCESS begin = { Convert(render_pass_desc.colors[i].load_op), {} };
        if (render_pass_desc.colors[i].load_op == RenderPassLoadOp::kClear) {
            begin.Clear.ClearValue.Color[0] = render_pass_desc.colors[i].clear_value.r;
            begin.Clear.ClearValue.Color[1] = render_pass_desc.colors[i].clear_value.g;
            begin.Clear.ClearValue.Color[2] = render_pass_desc.colors[i].clear_value.b;
            begin.Clear.ClearValue.Color[3] = render_pass_desc.colors[i].clear_value.a;
        }
        D3D12_RENDER_PASS_ENDING_ACCESS end = { Convert(render_pass_desc.colors[i].store_op), {} };
        om_rtv.push_back({ handle, begin, end });
    }

    D3D12_RENDER_PASS_DEPTH_STENCIL_DESC om_dsv = {};
    D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* om_dsv_ptr = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE om_dsv_handle = get_handle(render_pass_desc.depth_stencil_view);
    if (om_dsv_handle.ptr) {
        D3D12_RENDER_PASS_BEGINNING_ACCESS depth_begin = { Convert(render_pass_desc.depth.load_op), {} };
        D3D12_RENDER_PASS_ENDING_ACCESS depth_end = { Convert(render_pass_desc.depth.store_op), {} };
        D3D12_RENDER_PASS_BEGINNING_ACCESS stencil_begin = { Convert(render_pass_desc.stencil.load_op), {} };
        D3D12_RENDER_PASS_ENDING_ACCESS stencil_end = { Convert(render_pass_desc.stencil.store_op), {} };
        depth_begin.Clear.ClearValue.DepthStencil.Depth = render_pass_desc.depth.clear_value;
        stencil_begin.Clear.ClearValue.DepthStencil.Stencil = render_pass_desc.stencil.clear_value;
        om_dsv = { om_dsv_handle, depth_begin, stencil_begin, depth_end, stencil_end };
        om_dsv_ptr = &om_dsv;
    }

    command_list4_->BeginRenderPass(static_cast<uint32_t>(om_rtv.size()), om_rtv.data(), om_dsv_ptr,
                                    D3D12_RENDER_PASS_FLAG_NONE);

    if (shading_rate_image_view_ == render_pass_desc.shading_rate_image_view) {
        return;
    }

    if (render_pass_desc.shading_rate_image_view) {
        decltype(auto) dx_shading_rate_image =
            render_pass_desc.shading_rate_image_view->GetResource()->As<DXResource>();
        command_list5_->RSSetShadingRateImage(dx_shading_rate_image.GetResource());
    } else {
        command_list5_->RSSetShadingRateImage(nullptr);
    }
    shading_rate_image_view_ = render_pass_desc.shading_rate_image_view;
}

void DXCommandList::EndRenderPass()
{
    command_list4_->EndRenderPass();
}

void DXCommandList::BeginEvent(const std::string& name)
{
#if defined(_WIN32)
    if (device_.IsUnderGraphicsDebugger()) {
        PIXBeginEvent(command_list_.Get(), 0, nowide::widen(name).c_str());
    }
#endif
}

void DXCommandList::EndEvent()
{
#if defined(_WIN32)
    if (device_.IsUnderGraphicsDebugger()) {
        PIXEndEvent(command_list_.Get());
    }
#endif
}

void DXCommandList::Draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
{
    command_list_->DrawInstanced(vertex_count, instance_count, first_vertex, first_instance);
}

void DXCommandList::DrawIndexed(uint32_t index_count,
                                uint32_t instance_count,
                                uint32_t first_index,
                                int32_t vertex_offset,
                                uint32_t first_instance)
{
    command_list_->DrawIndexedInstanced(index_count, instance_count, first_index, vertex_offset, first_instance);
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
        dx_count_buffer = count_buffer->As<DXResource>().GetResource();
    } else {
        assert(count_buffer_offset == 0);
    }
    command_list_->ExecuteIndirect(device_.GetCommandSignature(type, stride), max_draw_count,
                                   dx_argument_buffer.GetResource(), argument_buffer_offset, dx_count_buffer,
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
    command_list_->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z);
}

void DXCommandList::DispatchIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset)
{
    ExecuteIndirect(D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH, argument_buffer, argument_buffer_offset, {}, 0, 1,
                    sizeof(DispatchIndirectCommand));
}

void DXCommandList::DispatchMesh(uint32_t thread_group_count_x,
                                 uint32_t thread_group_count_y,
                                 uint32_t thread_group_count_z)
{
    command_list6_->DispatchMesh(thread_group_count_x, thread_group_count_y, thread_group_count_z);
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

    command_list4_->DispatchRays(&dispatch_rays_desc);
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

        assert(barrier.base_mip_level + barrier.level_count <= dx_resource.GetResourceDesc().MipLevels);
        assert(barrier.base_array_layer + barrier.layer_count <= dx_resource.GetResourceDesc().DepthOrArraySize);

        if (barrier.base_mip_level == 0 && barrier.level_count == dx_resource.GetResourceDesc().MipLevels &&
            barrier.base_array_layer == 0 && barrier.layer_count == dx_resource.GetResourceDesc().DepthOrArraySize) {
            dx_barriers.emplace_back(
                CD3DX12_RESOURCE_BARRIER::Transition(dx_resource.GetResource(), dx_state_before, dx_state_after));
        } else {
            for (uint32_t i = barrier.base_mip_level; i < barrier.base_mip_level + barrier.level_count; ++i) {
                for (uint32_t j = barrier.base_array_layer; j < barrier.base_array_layer + barrier.layer_count; ++j) {
                    uint32_t subresource = i + j * dx_resource.GetResourceDesc().MipLevels;
                    dx_barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(
                        dx_resource.GetResource(), dx_state_before, dx_state_after, subresource));
                }
            }
        }
    }
    if (!dx_barriers.empty()) {
        command_list_->ResourceBarrier(dx_barriers.size(), dx_barriers.data());
    }
}

void DXCommandList::UAVResourceBarrier(const std::shared_ptr<Resource>& resource)
{
    D3D12_RESOURCE_BARRIER uav_barrier = {};
    uav_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    if (resource) {
        decltype(auto) dx_resource = resource->As<DXResource>();
        uav_barrier.UAV.pResource = dx_resource.GetResource();
    }
    command_list4_->ResourceBarrier(1, &uav_barrier);
}

void DXCommandList::SetViewport(float x, float y, float width, float height)
{
    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = x;
    viewport.TopLeftY = y;
    viewport.Width = width;
    viewport.Height = height;
    viewport.MinDepth = 0.0;
    viewport.MaxDepth = 1.0;
    command_list_->RSSetViewports(1, &viewport);
}

void DXCommandList::SetScissorRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
{
    D3D12_RECT rect = { .left = static_cast<int32_t>(left),
                        .top = static_cast<int32_t>(top),
                        .right = static_cast<int32_t>(right),
                        .bottom = static_cast<int32_t>(bottom) };
    command_list_->RSSetScissorRects(1, &rect);
}

void DXCommandList::IASetIndexBuffer(const std::shared_ptr<Resource>& resource, uint64_t offset, gli::format format)
{
    DXGI_FORMAT dx_format = static_cast<DXGI_FORMAT>(gli::dx().translate(format).DXGIFormat.DDS);
    decltype(auto) dx_resource = resource->As<DXResource>();
    D3D12_INDEX_BUFFER_VIEW index_buffer_view = {
        .BufferLocation = dx_resource.GetResource()->GetGPUVirtualAddress() + offset,
        .SizeInBytes = static_cast<uint32_t>(dx_resource.GetResourceDesc().Width - offset),
        .Format = dx_format,
    };
    command_list_->IASetIndexBuffer(&index_buffer_view);
}

void DXCommandList::IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource, uint64_t offset)
{
    if (state_ && state_->GetPipelineType() == PipelineType::kGraphics) {
        decltype(auto) dx_state = state_->As<DXGraphicsPipeline>();
        auto& strides = dx_state.GetStrideMap();
        auto it = strides.find(slot);
        if (it != strides.end()) {
            IASetVertexBufferImpl(slot, resource, offset, it->second);
        } else {
            IASetVertexBufferImpl(slot, nullptr, 0, 0);
        }
    }
    lazy_vertex_[slot] = { resource, offset };
}

void DXCommandList::IASetVertexBufferImpl(uint32_t slot,
                                          const std::shared_ptr<Resource>& resource,
                                          uint64_t offset,
                                          uint32_t stride)
{
    if (!resource) {
        D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view = {};
        command_list_->IASetVertexBuffers(slot, 1, &vertex_buffer_view);
        return;
    }

    decltype(auto) dx_resource = resource->As<DXResource>();
    D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view = {
        .BufferLocation = dx_resource.GetResource()->GetGPUVirtualAddress() + offset,
        .SizeInBytes = static_cast<uint32_t>(dx_resource.GetResourceDesc().Width - offset),
        .StrideInBytes = stride,
    };
    command_list_->IASetVertexBuffers(slot, 1, &vertex_buffer_view);
}

void DXCommandList::RSSetShadingRate(ShadingRate shading_rate, const std::array<ShadingRateCombiner, 2>& combiners)
{
    command_list5_->RSSetShadingRate(static_cast<D3D12_SHADING_RATE>(shading_rate),
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
        acceleration_structure_desc.SourceAccelerationStructureData = dx_src.GetAccelerationStructureAddress();
        acceleration_structure_desc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
    }
    acceleration_structure_desc.DestAccelerationStructureData = dx_dst.GetAccelerationStructureAddress();
    acceleration_structure_desc.ScratchAccelerationStructureData =
        dx_scratch.GetResource()->GetGPUVirtualAddress() + scratch_offset;
    command_list4_->BuildRaytracingAccelerationStructure(&acceleration_structure_desc, 0, nullptr);
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
    inputs.InstanceDescs = dx_instance_data.GetResource()->GetGPUVirtualAddress() + instance_offset;
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
        NOTREACHED();
    }
    command_list4_->CopyRaytracingAccelerationStructure(dx_dst.GetAccelerationStructureAddress(),
                                                        dx_src.GetAccelerationStructureAddress(), dx_mode);
}

void DXCommandList::CopyBuffer(const std::shared_ptr<Resource>& src_buffer,
                               const std::shared_ptr<Resource>& dst_buffer,
                               const std::vector<BufferCopyRegion>& regions)
{
    decltype(auto) dx_src_buffer = src_buffer->As<DXResource>();
    decltype(auto) dx_dst_buffer = dst_buffer->As<DXResource>();
    for (const auto& region : regions) {
        command_list_->CopyBufferRegion(dx_dst_buffer.GetResource(), region.dst_offset, dx_src_buffer.GetResource(),
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
        dst.pResource = dx_dst_texture.GetResource();
        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst.SubresourceIndex = region.texture_array_layer * dx_dst_texture.GetLevelCount() + region.texture_mip_level;

        D3D12_TEXTURE_COPY_LOCATION src = {};
        src.pResource = dx_src_buffer.GetResource();
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

        command_list_->CopyTextureRegion(&dst, region.texture_offset.x, region.texture_offset.y,
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
        dst.pResource = dx_dst_texture.GetResource();
        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst.SubresourceIndex = region.dst_array_layer * dx_dst_texture.GetLevelCount() + region.dst_mip_level;

        D3D12_TEXTURE_COPY_LOCATION src = {};
        src.pResource = dx_src_texture.GetResource();
        src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        src.SubresourceIndex = region.src_array_layer * dx_src_texture.GetLevelCount() + region.src_mip_level;

        D3D12_BOX src_box = {};
        src_box.left = region.src_offset.x;
        src_box.top = region.src_offset.y;
        src_box.front = region.src_offset.z;
        src_box.right = region.src_offset.x + region.extent.width;
        src_box.bottom = region.src_offset.y + region.extent.height;
        src_box.back = region.src_offset.z + region.extent.depth;

        command_list_->CopyTextureRegion(&dst, region.dst_offset.x, region.dst_offset.y, region.dst_offset.z, &src,
                                         &src_box);
    }
}

void DXCommandList::WriteAccelerationStructuresProperties(
    const std::vector<std::shared_ptr<Resource>>& acceleration_structures,
    const std::shared_ptr<QueryHeap>& query_heap,
    uint32_t first_query)
{
    assert(query_heap->GetType() == QueryHeapType::kAccelerationStructureCompactedSize);
    decltype(auto) dx_query_heap = query_heap->As<DXRayTracingQueryHeap>();
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC desc = {};
    desc.InfoType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE;
    desc.DestBuffer = dx_query_heap.GetResource()->GetGPUVirtualAddress() + first_query * sizeof(uint64_t);
    std::vector<D3D12_GPU_VIRTUAL_ADDRESS> dx_acceleration_structures;
    dx_acceleration_structures.reserve(acceleration_structures.size());
    for (const auto& acceleration_structure : acceleration_structures) {
        dx_acceleration_structures.emplace_back(
            acceleration_structure->As<DXResource>().GetAccelerationStructureAddress());
    }
    command_list4_->EmitRaytracingAccelerationStructurePostbuildInfo(&desc, dx_acceleration_structures.size(),
                                                                     dx_acceleration_structures.data());
}

void DXCommandList::ResolveQueryData(const std::shared_ptr<QueryHeap>& query_heap,
                                     uint32_t first_query,
                                     uint32_t query_count,
                                     const std::shared_ptr<Resource>& dst_buffer,
                                     uint64_t dst_offset)
{
    assert(query_heap->GetType() == QueryHeapType::kAccelerationStructureCompactedSize);
    decltype(auto) dx_query_heap = query_heap->As<DXRayTracingQueryHeap>();
    decltype(auto) dx_dst_buffer = dst_buffer->As<DXResource>();
    auto common_to_copy_barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        dx_query_heap.GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE, 0);
    command_list_->ResourceBarrier(1, &common_to_copy_barrier);
    command_list_->CopyBufferRegion(dx_dst_buffer.GetResource(), dst_offset, dx_query_heap.GetResource(),
                                    first_query * sizeof(uint64_t), query_count * sizeof(uint64_t));
    auto copy_to_common_barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        dx_query_heap.GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON, 0);
    command_list_->ResourceBarrier(1, &copy_to_common_barrier);
}

ComPtr<ID3D12GraphicsCommandList> DXCommandList::GetCommandList()
{
    return command_list_;
}

void DXCommandList::SetName(const std::string& name)
{
    command_list_->SetName(nowide::widen(name).c_str());
}
