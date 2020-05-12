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
        srv_desc.Buffer.FirstElement = 0;
        srv_desc.Buffer.NumElements = static_cast<uint32_t>(res_desc.Width / view_desc.stride);
        srv_desc.Buffer.StructureByteStride = view_desc.stride;
    }
    else
    {
        srv_desc.Format = res_desc.Format;
        /*if (IsTypeless(srv_desc.Format))
        {
            switch (binding_desc.ReturnType)
            {
            case D3D_RETURN_TYPE_FLOAT:
                srv_desc.Format = FloatFromTypeless(srv_desc.Format);
                break;
            case D3D_RETURN_TYPE_UINT:
                srv_desc.Format = UintFromTypeless(srv_desc.Format);
                break;
            case D3D_RETURN_TYPE_SINT:
                srv_desc.Format = SintFromTypeless(srv_desc.Format);
                break;
            default:
                assert(false);
            }
        }*/

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
            setup_mips(srv_desc.Texture2D.MostDetailedMip, srv_desc.Texture2D.MipLevels);
            break;
        }
        case ResourceDimension::kTexture2DArray:
        {
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
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
        default:
        {
            /*if (binding_desc.Type == D3D_SIT_RTACCELERATIONSTRUCTURE)
            {
                srv_desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
            }
            else
            {
                assert(false);
            }*/
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
{
    m_handle = m_device.GetDescriptorPool().AllocateDescriptor(view_desc.res_type);
    if (!resource)
        return;

    DXResource& dx_resource = static_cast<DXResource&>(*resource);
    switch (view_desc.res_type)
    {
    case ResourceType::kSrv:
        CreateSrv(view_desc, dx_resource, *m_handle);
        break;
    case ResourceType::kUav:
        CreateUAV(view_desc, dx_resource, *m_handle);
        break;
    case ResourceType::kCbv:
        CreateCBV(view_desc, dx_resource, *m_handle);
        break;
    case ResourceType::kSampler:
        CreateSampler(view_desc, dx_resource, *m_handle);
        break;
    case ResourceType::kRtv:
        CreateRTV(view_desc, dx_resource, *m_handle);
        break;
    case ResourceType::kDsv:
        CreateDSV(view_desc, dx_resource, *m_handle);
        break;
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

void DXView::CreateSrv(const ViewDesc& view_desc, const DXResource& res, DXDescriptorHandle& m_handle)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = DX12GeSRVDesc(view_desc, res.default_res->GetDesc());
    if (srv_desc.ViewDimension != D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE)
    {
        m_device.GetDevice()->CreateShaderResourceView(res.default_res.Get(), &srv_desc, m_handle.GetCpuHandle());
    }
    else
    {
        srv_desc.RaytracingAccelerationStructure.Location = res.default_res->GetGPUVirtualAddress();
        m_device.GetDevice()->CreateShaderResourceView(nullptr, &srv_desc, m_handle.GetCpuHandle());
    }
}

void DXView::CreateUAV(const ViewDesc& view_desc, const DXResource& res, DXDescriptorHandle& m_handle)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = DX12GetUAVDesc(view_desc, res.default_res->GetDesc());
    m_device.GetDevice()->CreateUnorderedAccessView(res.default_res.Get(), nullptr, &uav_desc, m_handle.GetCpuHandle());
}

void DXView::CreateRTV(const ViewDesc& view_desc, const DXResource& res, DXDescriptorHandle& m_handle)
{
    D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = DX12GetRTVDesc(view_desc, res.default_res->GetDesc());
    m_format = rtv_desc.Format;
    m_device.GetDevice()->CreateRenderTargetView(res.default_res.Get(), &rtv_desc, m_handle.GetCpuHandle());
}

void DXView::CreateDSV(const ViewDesc& view_desc, const DXResource& res, DXDescriptorHandle& m_handle)
{
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = DX12GetDSVDesc(view_desc, res.default_res->GetDesc());
    m_device.GetDevice()->CreateDepthStencilView(res.default_res.Get(), &dsv_desc, m_handle.GetCpuHandle());
}

void DXView::CreateCBV(const ViewDesc& view_desc, const DXResource& res, DXDescriptorHandle& m_handle)
{
    D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
    desc.BufferLocation = res.default_res->GetGPUVirtualAddress();
    desc.SizeInBytes = (res.default_res->GetDesc().Width + 255) & ~255;
    m_device.GetDevice()->CreateConstantBufferView(&desc, m_handle.GetCpuHandle());
}

void DXView::CreateSampler(const ViewDesc& view_desc, const DXResource& res, DXDescriptorHandle& m_handle)
{
    m_device.GetDevice()->CreateSampler(&res.sampler_desc, m_handle.GetCpuHandle());
}
