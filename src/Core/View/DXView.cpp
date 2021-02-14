#include "View/DXView.h"
#include <Device/DXDevice.h>
#include <Utilities/DXGIFormatHelper.h>
#include <cassert>
#include <d3d12.h>

D3D12_SHADER_RESOURCE_VIEW_DESC DX12GeSRVDesc(const ViewDesc& view_desc, const D3D12_RESOURCE_DESC& res_desc)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    if (view_desc.dimension == ResourceDimension::kBuffer)
    {
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.Buffer.FirstElement = view_desc.offset / view_desc.stride;
        srv_desc.Buffer.NumElements = static_cast<uint32_t>((res_desc.Width - view_desc.offset) / view_desc.stride);
        srv_desc.Buffer.StructureByteStride = view_desc.stride;
    }
    else
    {
        srv_desc.Format = res_desc.Format;
        if (IsTypelessDepthStencil(srv_desc.Format))
        {
            if (view_desc.plane_slice == 0)
                srv_desc.Format = DepthReadFromTypeless(srv_desc.Format);
            else
                srv_desc.Format = StencilReadFromTypeless(srv_desc.Format);
        }

        auto setup_mips = [&](uint32_t& MostDetailedMip, uint32_t& MipLevels)
        {
            MostDetailedMip = view_desc.level;
            if (view_desc.count == -1)
                MipLevels = res_desc.MipLevels - MostDetailedMip;
            else
                MipLevels = view_desc.count;
        };

        switch (view_desc.dimension)
        {
        case ResourceDimension::kTexture1D:
        {
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
            setup_mips(srv_desc.Texture1D.MostDetailedMip, srv_desc.Texture1D.MipLevels);
            break;
        }
        case ResourceDimension::kTexture1DArray:
        {
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
            setup_mips(srv_desc.Texture1DArray.MostDetailedMip, srv_desc.Texture1DArray.MipLevels);
            srv_desc.Texture1DArray.FirstArraySlice = 0;
            srv_desc.Texture1DArray.ArraySize = res_desc.DepthOrArraySize;
            break;
        }
        case ResourceDimension::kTexture2D:
        {
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srv_desc.Texture2D.PlaneSlice = view_desc.plane_slice;
            setup_mips(srv_desc.Texture2D.MostDetailedMip, srv_desc.Texture2D.MipLevels);
            break;
        }
        case ResourceDimension::kTexture2DArray:
        {
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            srv_desc.Texture2DArray.PlaneSlice = view_desc.plane_slice;
            setup_mips(srv_desc.Texture2DArray.MostDetailedMip, srv_desc.Texture2DArray.MipLevels);
            srv_desc.Texture2DArray.FirstArraySlice = 0;
            srv_desc.Texture2DArray.ArraySize = res_desc.DepthOrArraySize;
            break;
        }
        case ResourceDimension::kTexture2DMS:
        {
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
            break;
        }
        case ResourceDimension::kTexture2DMSArray:
        {
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
            srv_desc.Texture2DMSArray.FirstArraySlice = 0;
            srv_desc.Texture2DMSArray.ArraySize = res_desc.DepthOrArraySize;
            break;
        }
        case ResourceDimension::kTexture3D:
        {
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
            setup_mips(srv_desc.Texture3D.MostDetailedMip, srv_desc.Texture3D.MipLevels);
            break;
        }
        case ResourceDimension::kTextureCube:
        {
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            setup_mips(srv_desc.TextureCube.MostDetailedMip, srv_desc.TextureCube.MipLevels);
            break;
        }
        case ResourceDimension::kTextureCubeArray:
        {
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
            setup_mips(srv_desc.TextureCubeArray.MostDetailedMip, srv_desc.TextureCubeArray.MipLevels);
            srv_desc.TextureCubeArray.First2DArrayFace = 0;
            srv_desc.TextureCubeArray.NumCubes = res_desc.DepthOrArraySize / 6;
            break;
        }
        case ResourceDimension::kRaytracingAccelerationStructure:
        {
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
            break;
        }
        default:
        {
            assert(false);
            break;
        }
        }
    }
    return srv_desc;
}

D3D12_UNORDERED_ACCESS_VIEW_DESC DX12GetUAVDesc(const ViewDesc& view_desc, const D3D12_RESOURCE_DESC& res_desc)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
    if (view_desc.dimension == ResourceDimension::kBuffer)
    {
        uav_desc.Format = DXGI_FORMAT_UNKNOWN;
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uav_desc.Buffer.FirstElement = 0;
        uav_desc.Buffer.NumElements = static_cast<uint32_t>(res_desc.Width / view_desc.stride);
        uav_desc.Buffer.StructureByteStride = view_desc.stride;
    }
    else
    {
        uav_desc.Format = res_desc.Format;
        /*if (IsTypeless(uav_desc.Format))
        {
            switch (binding_desc.ReturnType)
            {
            case D3D_RETURN_TYPE_FLOAT:
                uav_desc.Format = FloatFromTypeless(uav_desc.Format);
                break;
            case D3D_RETURN_TYPE_UINT:
                uav_desc.Format = UintFromTypeless(uav_desc.Format);
                break;
            case D3D_RETURN_TYPE_SINT:
                uav_desc.Format = SintFromTypeless(uav_desc.Format);
                break;
            default:
                assert(false);
            }
        }*/
        switch (view_desc.dimension)
        {
        case ResourceDimension::kTexture1D:
        {
            uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
            uav_desc.Texture1D.MipSlice = view_desc.level;
            break;
        }
        case ResourceDimension::kTexture1DArray:
        {
            uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
            uav_desc.Texture1DArray.MipSlice = view_desc.level;
            uav_desc.Texture1DArray.FirstArraySlice = 0;
            uav_desc.Texture1DArray.ArraySize = res_desc.DepthOrArraySize;
            break;
        }
        case ResourceDimension::kTexture2D:
        {
            uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uav_desc.Texture2D.MipSlice = view_desc.level;
            break;
        }
        case ResourceDimension::kTexture2DArray:
        {
            uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            uav_desc.Texture2DArray.MipSlice = view_desc.level;
            uav_desc.Texture2DArray.FirstArraySlice = 0;
            uav_desc.Texture2DArray.ArraySize = res_desc.DepthOrArraySize;
            break;
        }
        case ResourceDimension::kTexture3D:
        {
            uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
            uav_desc.Texture3D.MipSlice = view_desc.level;
            break;
        }
        default:
            assert(false);
            break;
        }
    }
    return uav_desc;
}

D3D12_RENDER_TARGET_VIEW_DESC DX12GetRTVDesc(const ViewDesc& view_desc, const D3D12_RESOURCE_DESC& res_desc)
{
    bool arrayed = res_desc.DepthOrArraySize > 1;
    bool ms = res_desc.SampleDesc.Count > 1;

    D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
    rtv_desc.Format = res_desc.Format;
    /*if (IsTypeless(rtv_desc.Format))
    {
        switch (binding_desc.ComponentType)
        {
        case D3D_REGISTER_COMPONENT_FLOAT32:
            rtv_desc.Format = FloatFromTypeless(rtv_desc.Format);
            break;
        case D3D_REGISTER_COMPONENT_UINT32:
            rtv_desc.Format = UintFromTypeless(rtv_desc.Format);
            break;
        case D3D_REGISTER_COMPONENT_SINT32:
            rtv_desc.Format = SintFromTypeless(rtv_desc.Format);
            break;
        default:
            assert(false);
        }
    }*/

    if (!arrayed && !ms)
    {
        rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtv_desc.Texture2D.MipSlice = view_desc.level;
    }
    else if (arrayed && !ms)
    {
        rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        rtv_desc.Texture2DArray.MipSlice = view_desc.level;
        rtv_desc.Texture2DArray.FirstArraySlice = 0;
        rtv_desc.Texture2DArray.ArraySize = res_desc.DepthOrArraySize;
    }
    else if (!arrayed && ms)
    {
        rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
    }
    else if (arrayed && ms)
    {
        rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
        rtv_desc.Texture2DMSArray.FirstArraySlice = 0;
        rtv_desc.Texture2DMSArray.ArraySize = res_desc.DepthOrArraySize;
    }
    return rtv_desc;
}

D3D12_DEPTH_STENCIL_VIEW_DESC DX12GetDSVDesc(const ViewDesc& view_desc, const D3D12_RESOURCE_DESC& res_desc)
{
    bool arrayed = res_desc.DepthOrArraySize > 1;
    bool ms = res_desc.SampleDesc.Count > 1;

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
    dsv_desc.Format = DepthStencilFromTypeless(res_desc.Format);
    if (!arrayed && !ms)
    {
        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsv_desc.Texture2D.MipSlice = view_desc.level;
    }
    else if (arrayed && !ms)
    {
        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        dsv_desc.Texture2DArray.MipSlice = view_desc.level;
        dsv_desc.Texture2DArray.FirstArraySlice = 0;
        dsv_desc.Texture2DArray.ArraySize = res_desc.DepthOrArraySize;
    }
    else if (!arrayed && ms)
    {
        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
    }
    else if (arrayed && ms)
    {
        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
        dsv_desc.Texture2DMSArray.FirstArraySlice = 0;
        dsv_desc.Texture2DMSArray.ArraySize = res_desc.DepthOrArraySize;
    }
    return dsv_desc;
}

DXView::DXView(DXDevice& device, const std::shared_ptr <Resource>& resource, const ViewDesc& view_desc)
    : m_device(device)
    , m_view_desc(view_desc)
{
    m_resource = resource;
    if (view_desc.view_type == ViewType::kShadingRateSource)
        return;
    m_handle = m_device.GetCPUDescriptorPool().AllocateDescriptor(view_desc.view_type);

    DXResource* dx_resource = nullptr;
    if (resource)
        dx_resource = &resource->As<DXResource>();

    switch (view_desc.view_type)
    {
    case ViewType::kShaderResource:
        CreateSrv(view_desc, dx_resource, *m_handle);
        break;
    case ViewType::kUnorderedAccess:
        CreateUAV(view_desc, dx_resource, *m_handle);
        break;
    case ViewType::kConstantBuffer:
        CreateCBV(view_desc, dx_resource, *m_handle);
        break;
    case ViewType::kSampler:
        CreateSampler(view_desc, dx_resource, *m_handle);
        break;
    case ViewType::kRenderTarget:
        CreateRTV(view_desc, dx_resource, *m_handle);
        break;
    case ViewType::kDepthStencil:
        CreateDSV(view_desc, dx_resource, *m_handle);
        break;
    }

    if (view_desc.bindless)
    {
        assert(view_desc.view_type != ViewType::kUnknown);
        assert(view_desc.view_type != ViewType::kSampler);
        assert(view_desc.view_type != ViewType::kRenderTarget);
        assert(view_desc.view_type != ViewType::kDepthStencil);
        m_range = std::make_shared<DXGPUDescriptorPoolRange>(m_device.GetGPUDescriptorPool().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1));
        m_range->CopyCpuHandle(0, m_handle->GetCpuHandle());
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE DXView::GetHandle()
{
    return m_handle->GetCpuHandle();
}

DXGI_FORMAT DXView::GetFormat() const
{
    return m_format;
}

void DXView::CreateSrv(const ViewDesc& view_desc, const DXResource* res, DXCPUDescriptorHandle& m_handle)
{
    if (!res)
        return;
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = DX12GeSRVDesc(view_desc, res->resource->GetDesc());
    if (srv_desc.ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE)
    {
        srv_desc.RaytracingAccelerationStructure.Location = res->resource->GetGPUVirtualAddress();
        m_device.GetDevice()->CreateShaderResourceView(nullptr, &srv_desc, m_handle.GetCpuHandle());
    }
    else
    {
        m_device.GetDevice()->CreateShaderResourceView(res->resource.Get(), &srv_desc, m_handle.GetCpuHandle());
    }
}

void DXView::CreateUAV(const ViewDesc& view_desc, const DXResource* res, DXCPUDescriptorHandle& m_handle)
{
    if (!res)
        return;
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = DX12GetUAVDesc(view_desc, res->resource->GetDesc());
    m_device.GetDevice()->CreateUnorderedAccessView(res->resource.Get(), nullptr, &uav_desc, m_handle.GetCpuHandle());
}

void DXView::CreateRTV(const ViewDesc& view_desc, const DXResource* res, DXCPUDescriptorHandle& m_handle)
{
    if (res)
    {
        D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = DX12GetRTVDesc(view_desc, res->resource->GetDesc());
        m_format = rtv_desc.Format;
        m_device.GetDevice()->CreateRenderTargetView(res->resource.Get(), &rtv_desc, m_handle.GetCpuHandle());
    }
    else
    {
        D3D12_RESOURCE_DESC res_desc = {};
        res_desc.Format = DXGI_FORMAT_R8G8B8A8_SNORM;
        D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = DX12GetRTVDesc(view_desc, res_desc);
        m_format = rtv_desc.Format;
        m_device.GetDevice()->CreateRenderTargetView(nullptr, &rtv_desc, m_handle.GetCpuHandle());
    }
}

void DXView::CreateDSV(const ViewDesc& view_desc, const DXResource* res, DXCPUDescriptorHandle& m_handle)
{
    if (res)
    {
        D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = DX12GetDSVDesc(view_desc, res->resource->GetDesc());
        m_device.GetDevice()->CreateDepthStencilView(res->resource.Get(), &dsv_desc, m_handle.GetCpuHandle());
    }
}

void DXView::CreateCBV(const ViewDesc& view_desc, const DXResource* res, DXCPUDescriptorHandle& m_handle)
{
    if (res)
    {
        D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
        desc.BufferLocation = res->resource->GetGPUVirtualAddress();
        desc.SizeInBytes = (res->resource->GetDesc().Width + 255) & ~255;
        m_device.GetDevice()->CreateConstantBufferView(&desc, m_handle.GetCpuHandle());
    }
}

void DXView::CreateSampler(const ViewDesc& view_desc, const DXResource* res, DXCPUDescriptorHandle& m_handle)
{
    if (res)
    {
        m_device.GetDevice()->CreateSampler(&res->sampler_desc, m_handle.GetCpuHandle());
    }
}

std::shared_ptr<Resource> DXView::GetResource()
{
    return m_resource;
}

uint32_t DXView::GetDescriptorId() const
{
    if (m_range)
        return m_range->GetOffset();
    return -1;
}

uint32_t DXView::GetBaseMipLevel() const
{
    return m_view_desc.level;
}

uint32_t DXView::GetLevelCount() const
{
    return std::min<uint32_t>(m_view_desc.count, m_resource->GetLevelCount() - m_view_desc.level);
}

uint32_t DXView::GetBaseArrayLayer() const
{
    return 0;
}

uint32_t DXView::GetLayerCount() const
{
    return m_resource->GetLayerCount();
}
