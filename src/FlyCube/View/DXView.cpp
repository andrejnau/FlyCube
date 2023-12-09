#include "View/DXView.h"

#include "Device/DXDevice.h"
#include "Utilities/DXGIFormatHelper.h"

#include <directx/d3d12.h>
#include <gli/gli.hpp>

#include <cassert>

DXView::DXView(DXDevice& device, const std::shared_ptr<DXResource>& resource, const ViewDesc& m_view_desc)
    : m_device(device)
    , m_resource(resource)
    , m_view_desc(m_view_desc)
{
    if (m_view_desc.view_type == ViewType::kShadingRateSource) {
        return;
    }

    m_handle = m_device.GetCPUDescriptorPool().AllocateDescriptor(m_view_desc.view_type);

    if (m_resource) {
        CreateView();
    }

    if (m_view_desc.bindless) {
        assert(m_view_desc.view_type != ViewType::kUnknown);
        assert(m_view_desc.view_type != ViewType::kRenderTarget);
        assert(m_view_desc.view_type != ViewType::kDepthStencil);
        if (m_view_desc.view_type == ViewType::kSampler) {
            m_range = std::make_shared<DXGPUDescriptorPoolRange>(
                m_device.GetGPUDescriptorPool().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1));
        } else {
            m_range = std::make_shared<DXGPUDescriptorPoolRange>(
                m_device.GetGPUDescriptorPool().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1));
        }
        m_range->CopyCpuHandle(0, m_handle->GetCpuHandle());
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE DXView::GetHandle()
{
    return m_handle->GetCpuHandle();
}

void DXView::CreateView()
{
    switch (m_view_desc.view_type) {
    case ViewType::kTexture:
    case ViewType::kBuffer:
    case ViewType::kStructuredBuffer:
        CreateSRV();
        break;
    case ViewType::kAccelerationStructure:
        CreateRAS();
        break;
    case ViewType::kRWTexture:
    case ViewType::kRWBuffer:
    case ViewType::kRWStructuredBuffer:
        CreateUAV();
        break;
    case ViewType::kConstantBuffer:
        CreateCBV();
        break;
    case ViewType::kSampler:
        CreateSampler();
        break;
    case ViewType::kRenderTarget:
        CreateRTV();
        break;
    case ViewType::kDepthStencil:
        CreateDSV();
        break;
    default:
        assert(false);
        break;
    }
}

void DXView::CreateSRV()
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Format = m_resource->desc.Format;

    if (IsTypelessDepthStencil(srv_desc.Format)) {
        if (m_view_desc.plane_slice == 0) {
            srv_desc.Format = DepthReadFromTypeless(srv_desc.Format);
        } else {
            srv_desc.Format = StencilReadFromTypeless(srv_desc.Format);
        }
    }

    auto setup_mips = [&](uint32_t& most_detailed_mip, uint32_t& mip_levels) {
        most_detailed_mip = GetBaseMipLevel();
        mip_levels = GetLevelCount();
    };

    switch (m_view_desc.dimension) {
    case ViewDimension::kTexture1D: {
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
        setup_mips(srv_desc.Texture1D.MostDetailedMip, srv_desc.Texture1D.MipLevels);
        break;
    }
    case ViewDimension::kTexture1DArray: {
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
        srv_desc.Texture1DArray.FirstArraySlice = GetBaseArrayLayer();
        srv_desc.Texture1DArray.ArraySize = GetLayerCount();
        setup_mips(srv_desc.Texture1DArray.MostDetailedMip, srv_desc.Texture1DArray.MipLevels);
        break;
    }
    case ViewDimension::kTexture2D: {
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Texture2D.PlaneSlice = m_view_desc.plane_slice;
        setup_mips(srv_desc.Texture2D.MostDetailedMip, srv_desc.Texture2D.MipLevels);
        break;
    }
    case ViewDimension::kTexture2DArray: {
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        srv_desc.Texture2DArray.PlaneSlice = m_view_desc.plane_slice;
        srv_desc.Texture2DArray.FirstArraySlice = GetBaseArrayLayer();
        srv_desc.Texture2DArray.ArraySize = GetLayerCount();
        setup_mips(srv_desc.Texture2DArray.MostDetailedMip, srv_desc.Texture2DArray.MipLevels);
        break;
    }
    case ViewDimension::kTexture2DMS: {
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
        break;
    }
    case ViewDimension::kTexture2DMSArray: {
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
        srv_desc.Texture2DMSArray.FirstArraySlice = GetBaseArrayLayer();
        srv_desc.Texture2DMSArray.ArraySize = GetLayerCount();
        break;
    }
    case ViewDimension::kTexture3D: {
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        setup_mips(srv_desc.Texture3D.MostDetailedMip, srv_desc.Texture3D.MipLevels);
        break;
    }
    case ViewDimension::kTextureCube: {
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        setup_mips(srv_desc.TextureCube.MostDetailedMip, srv_desc.TextureCube.MipLevels);
        break;
    }
    case ViewDimension::kTextureCubeArray: {
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        srv_desc.TextureCubeArray.First2DArrayFace = GetBaseArrayLayer() / 6;
        srv_desc.TextureCubeArray.NumCubes = GetLayerCount() / 6;
        setup_mips(srv_desc.TextureCubeArray.MostDetailedMip, srv_desc.TextureCubeArray.MipLevels);
        break;
    }
    case ViewDimension::kBuffer: {
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        uint32_t stride = 0;
        if (m_view_desc.view_type == ViewType::kBuffer) {
            srv_desc.Format = static_cast<DXGI_FORMAT>(gli::dx().translate(m_view_desc.buffer_format).DXGIFormat.DDS);
            stride = gli::detail::bits_per_pixel(m_view_desc.buffer_format) / 8;
        } else {
            assert(m_view_desc.view_type == ViewType::kStructuredBuffer);
            srv_desc.Buffer.StructureByteStride = m_view_desc.structure_stride;
            stride = srv_desc.Buffer.StructureByteStride;
        }
        uint64_t size = std::min(m_resource->desc.Width, m_view_desc.buffer_size);
        srv_desc.Buffer.FirstElement = m_view_desc.offset / stride;
        srv_desc.Buffer.NumElements = (size - m_view_desc.offset) / (stride);
        break;
    }
    default: {
        assert(false);
        break;
    }
    }

    m_device.GetDevice()->CreateShaderResourceView(m_resource->resource.Get(), &srv_desc, m_handle->GetCpuHandle());
}

void DXView::CreateRAS()
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    srv_desc.RaytracingAccelerationStructure.Location = m_resource->acceleration_structure_handle;
    m_device.GetDevice()->CreateShaderResourceView(nullptr, &srv_desc, m_handle->GetCpuHandle());
}

void DXView::CreateUAV()
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
    uav_desc.Format = m_resource->desc.Format;

    switch (m_view_desc.dimension) {
    case ViewDimension::kTexture1D: {
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
        uav_desc.Texture1D.MipSlice = GetBaseMipLevel();
        break;
    }
    case ViewDimension::kTexture1DArray: {
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
        uav_desc.Texture1DArray.FirstArraySlice = GetBaseArrayLayer();
        uav_desc.Texture1DArray.ArraySize = GetLayerCount();
        uav_desc.Texture1DArray.MipSlice = GetBaseMipLevel();
        break;
    }
    case ViewDimension::kTexture2D: {
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uav_desc.Texture2D.PlaneSlice = m_view_desc.plane_slice;
        uav_desc.Texture2D.MipSlice = GetBaseMipLevel();
        break;
    }
    case ViewDimension::kTexture2DArray: {
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        uav_desc.Texture2DArray.PlaneSlice = m_view_desc.plane_slice;
        uav_desc.Texture2DArray.FirstArraySlice = GetBaseArrayLayer();
        uav_desc.Texture2DArray.ArraySize = GetLayerCount();
        uav_desc.Texture2DArray.MipSlice = GetBaseMipLevel();
        break;
    }
    case ViewDimension::kTexture3D: {
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
        uav_desc.Texture3D.MipSlice = GetBaseMipLevel();
        break;
    }
    case ViewDimension::kBuffer: {
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uint32_t stride = 0;
        if (m_view_desc.view_type == ViewType::kRWBuffer) {
            uav_desc.Format = static_cast<DXGI_FORMAT>(gli::dx().translate(m_view_desc.buffer_format).DXGIFormat.DDS);
            stride = gli::detail::bits_per_pixel(m_view_desc.buffer_format) / 8;
        } else {
            assert(m_view_desc.view_type == ViewType::kRWStructuredBuffer);
            uav_desc.Buffer.StructureByteStride = m_view_desc.structure_stride;
            stride = uav_desc.Buffer.StructureByteStride;
        }
        uint64_t size = std::min(m_resource->desc.Width, m_view_desc.buffer_size);
        uav_desc.Buffer.FirstElement = m_view_desc.offset / stride;
        uav_desc.Buffer.NumElements = (size - m_view_desc.offset) / (stride);
        break;
    }
    default: {
        assert(false);
        break;
    }
    }

    m_device.GetDevice()->CreateUnorderedAccessView(m_resource->resource.Get(), nullptr, &uav_desc,
                                                    m_handle->GetCpuHandle());
}

void DXView::CreateRTV()
{
    D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
    rtv_desc.Format = m_resource->desc.Format;

    switch (m_view_desc.dimension) {
    case ViewDimension::kTexture1D: {
        rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
        rtv_desc.Texture1D.MipSlice = GetBaseMipLevel();
        break;
    }
    case ViewDimension::kTexture1DArray: {
        rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
        rtv_desc.Texture1DArray.FirstArraySlice = GetBaseArrayLayer();
        rtv_desc.Texture1DArray.ArraySize = GetLayerCount();
        rtv_desc.Texture1DArray.MipSlice = GetBaseMipLevel();
        break;
    }
    case ViewDimension::kTexture2D: {
        rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtv_desc.Texture2D.PlaneSlice = m_view_desc.plane_slice;
        rtv_desc.Texture2D.MipSlice = GetBaseMipLevel();
        break;
    }
    case ViewDimension::kTexture2DArray: {
        rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        rtv_desc.Texture2DArray.PlaneSlice = m_view_desc.plane_slice;
        rtv_desc.Texture2DArray.FirstArraySlice = GetBaseArrayLayer();
        rtv_desc.Texture2DArray.ArraySize = GetLayerCount();
        rtv_desc.Texture2DArray.MipSlice = GetBaseMipLevel();
        break;
    }
    case ViewDimension::kTexture2DMS: {
        rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
        break;
    }
    case ViewDimension::kTexture2DMSArray: {
        rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
        rtv_desc.Texture2DMSArray.FirstArraySlice = GetBaseArrayLayer();
        rtv_desc.Texture2DMSArray.ArraySize = GetLayerCount();
        break;
    }
    case ViewDimension::kTexture3D: {
        rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
        rtv_desc.Texture3D.MipSlice = GetBaseMipLevel();
        break;
    }
    default: {
        assert(false);
        break;
    }
    }

    m_device.GetDevice()->CreateRenderTargetView(m_resource->resource.Get(), &rtv_desc, m_handle->GetCpuHandle());
}

void DXView::CreateDSV()
{
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
    dsv_desc.Format = DepthStencilFromTypeless(m_resource->desc.Format);

    switch (m_view_desc.dimension) {
    case ViewDimension::kTexture1D: {
        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
        dsv_desc.Texture1D.MipSlice = GetBaseMipLevel();
        break;
    }
    case ViewDimension::kTexture1DArray: {
        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
        dsv_desc.Texture1DArray.FirstArraySlice = GetBaseArrayLayer();
        dsv_desc.Texture1DArray.ArraySize = GetLayerCount();
        dsv_desc.Texture1DArray.MipSlice = GetBaseMipLevel();
        break;
    }
    case ViewDimension::kTexture2D: {
        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsv_desc.Texture2D.MipSlice = GetBaseMipLevel();
        break;
    }
    case ViewDimension::kTexture2DArray: {
        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        dsv_desc.Texture2DArray.FirstArraySlice = GetBaseArrayLayer();
        dsv_desc.Texture2DArray.ArraySize = GetLayerCount();
        dsv_desc.Texture2DArray.MipSlice = GetBaseMipLevel();
        break;
    }
    case ViewDimension::kTexture2DMS: {
        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
        break;
    }
    case ViewDimension::kTexture2DMSArray: {
        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
        dsv_desc.Texture2DMSArray.FirstArraySlice = GetLayerCount();
        dsv_desc.Texture2DMSArray.ArraySize = GetLayerCount();
        break;
    }
    default: {
        assert(false);
        break;
    }
    }

    m_device.GetDevice()->CreateDepthStencilView(m_resource->resource.Get(), &dsv_desc, m_handle->GetCpuHandle());
}

void DXView::CreateCBV()
{
    D3D12_CONSTANT_BUFFER_VIEW_DESC cvb_desc = {};
    cvb_desc.BufferLocation = m_resource->resource->GetGPUVirtualAddress() + m_view_desc.offset;
    cvb_desc.SizeInBytes = std::min(m_resource->desc.Width, m_view_desc.buffer_size);
    assert(cvb_desc.SizeInBytes % 256 == 0);
    m_device.GetDevice()->CreateConstantBufferView(&cvb_desc, m_handle->GetCpuHandle());
}

void DXView::CreateSampler()
{
    m_device.GetDevice()->CreateSampler(&m_resource->sampler_desc, m_handle->GetCpuHandle());
}

std::shared_ptr<Resource> DXView::GetResource()
{
    return m_resource;
}

uint32_t DXView::GetDescriptorId() const
{
    if (m_range) {
        return m_range->GetOffset();
    }
    return -1;
}

uint32_t DXView::GetBaseMipLevel() const
{
    return m_view_desc.base_mip_level;
}

uint32_t DXView::GetLevelCount() const
{
    return std::min<uint32_t>(m_view_desc.level_count, m_resource->GetLevelCount() - m_view_desc.base_mip_level);
}

uint32_t DXView::GetBaseArrayLayer() const
{
    return m_view_desc.base_array_layer;
}

uint32_t DXView::GetLayerCount() const
{
    return std::min<uint32_t>(m_view_desc.layer_count, m_resource->GetLayerCount() - m_view_desc.base_array_layer);
}
