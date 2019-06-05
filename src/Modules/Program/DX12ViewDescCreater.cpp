#include "DX12ViewDescCreater.h"

static const UINT32 D3D_SIT_RTACCELERATIONSTRUCTURE = 12;

D3D12_SHADER_RESOURCE_VIEW_DESC DX12GeSRVDesc(const D3D12_SHADER_INPUT_BIND_DESC& binding_desc, const ViewDesc& view_desc, const DX12Resource& res)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    D3D12_RESOURCE_DESC desc = res.default_res->GetDesc();
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    if (binding_desc.Dimension == D3D_SRV_DIMENSION_BUFFER)
    {
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.Buffer.FirstElement = 0;
        srv_desc.Buffer.NumElements = static_cast<uint32_t>(desc.Width / res.stride);
        srv_desc.Buffer.StructureByteStride = res.stride;
    }
    else
    {
        srv_desc.Format = desc.Format;
        if (IsTypeless(srv_desc.Format))
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
        }

        auto setup_mips = [&](uint32_t& MostDetailedMip, uint32_t& MipLevels)
        {
            MostDetailedMip = view_desc.level;
            if (view_desc.count == -1)
                MipLevels = desc.MipLevels - MostDetailedMip;
            else
                MipLevels = view_desc.count;
        };

        switch (binding_desc.Dimension)
        {
        case D3D_SRV_DIMENSION_TEXTURE1D:
        {
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
            setup_mips(srv_desc.Texture1D.MostDetailedMip, srv_desc.Texture1D.MipLevels);
            break;
        }
        case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
        {
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
            setup_mips(srv_desc.Texture1DArray.MostDetailedMip, srv_desc.Texture1DArray.MipLevels);
            srv_desc.Texture1DArray.FirstArraySlice = 0;
            srv_desc.Texture1DArray.ArraySize = desc.DepthOrArraySize;
            break;
        }
        case D3D_SRV_DIMENSION_TEXTURE2D:
        {
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            setup_mips(srv_desc.Texture2D.MostDetailedMip, srv_desc.Texture2D.MipLevels);
            break;
        }
        case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
        {
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            setup_mips(srv_desc.Texture2DArray.MostDetailedMip, srv_desc.Texture2DArray.MipLevels);
            srv_desc.Texture2DArray.FirstArraySlice = 0;
            srv_desc.Texture2DArray.ArraySize = desc.DepthOrArraySize;
            break;
        }
        case D3D_SRV_DIMENSION_TEXTURE2DMS:
        {
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
            break;
        }
        case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
        {
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
            srv_desc.Texture2DMSArray.FirstArraySlice = 0;
            srv_desc.Texture2DMSArray.ArraySize = desc.DepthOrArraySize;
            break;
        }
        case D3D_SRV_DIMENSION_TEXTURE3D:
        {
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
            setup_mips(srv_desc.Texture3D.MostDetailedMip, srv_desc.Texture3D.MipLevels);
            break;
        }
        case D3D_SRV_DIMENSION_TEXTURECUBE:
        {
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            setup_mips(srv_desc.TextureCube.MostDetailedMip, srv_desc.TextureCube.MipLevels);
            break;
        }
        case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
        {
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
            setup_mips(srv_desc.TextureCubeArray.MostDetailedMip, srv_desc.TextureCubeArray.MipLevels);
            srv_desc.TextureCubeArray.First2DArrayFace = 0;
            srv_desc.TextureCubeArray.NumCubes = desc.DepthOrArraySize / 6;
            break;
        }
        default:
        {
            if (binding_desc.Type == D3D_SIT_RTACCELERATIONSTRUCTURE)
            {
                srv_desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
            }
            else
            {
                assert(false);
            }
            break;
        }
        }
    }
    return srv_desc;
}

D3D12_UNORDERED_ACCESS_VIEW_DESC DX12GetUAVDesc(const D3D12_SHADER_INPUT_BIND_DESC& binding_desc, const ViewDesc& view_desc, const DX12Resource& res)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
    D3D12_RESOURCE_DESC desc = res.default_res->GetDesc();
    if (binding_desc.Dimension == D3D_SRV_DIMENSION_BUFFER)
    {
        uav_desc.Format = DXGI_FORMAT_UNKNOWN;
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uav_desc.Buffer.FirstElement = 0;
        uav_desc.Buffer.NumElements = static_cast<uint32_t>(desc.Width / res.stride);
        uav_desc.Buffer.StructureByteStride = res.stride;
    }
    else
    {
        uav_desc.Format = desc.Format;
        if (IsTypeless(uav_desc.Format))
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
        }
        switch (binding_desc.Dimension)
        {
        case D3D_SRV_DIMENSION_TEXTURE1D:
        {
            uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
            uav_desc.Texture1D.MipSlice = view_desc.level;
            break;
        }
        case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
        {
            uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
            uav_desc.Texture1DArray.MipSlice = view_desc.level;
            uav_desc.Texture1DArray.FirstArraySlice = 0;
            uav_desc.Texture1DArray.ArraySize = desc.DepthOrArraySize;
            break;
        }
        case D3D_SRV_DIMENSION_TEXTURE2D:
        {
            uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uav_desc.Texture2D.MipSlice = view_desc.level;
            break;
        }
        case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
        {
            uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            uav_desc.Texture2DArray.MipSlice = view_desc.level;
            uav_desc.Texture2DArray.FirstArraySlice = 0;
            uav_desc.Texture2DArray.ArraySize = desc.DepthOrArraySize;
            break;
        }
        case D3D_SRV_DIMENSION_TEXTURE3D:
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

D3D12_RENDER_TARGET_VIEW_DESC DX12GetRTVDesc(const D3D12_SIGNATURE_PARAMETER_DESC& binding_desc, const ViewDesc& view_desc, const DX12Resource& res)
{
    D3D12_RESOURCE_DESC desc = res.default_res->GetDesc();
    bool arrayed = desc.DepthOrArraySize > 1;
    bool ms = desc.SampleDesc.Count > 1;

    D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
    rtv_desc.Format = desc.Format;
    if (IsTypeless(rtv_desc.Format))
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
    }

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
        rtv_desc.Texture2DArray.ArraySize = desc.DepthOrArraySize;
    }
    else if (!arrayed && ms)
    {
        rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
    }
    else if (arrayed && ms)
    {
        rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
        rtv_desc.Texture2DMSArray.FirstArraySlice = 0;
        rtv_desc.Texture2DMSArray.ArraySize = desc.DepthOrArraySize;
    }
    return rtv_desc;
}

D3D12_DEPTH_STENCIL_VIEW_DESC DX12GetDSVDesc(const ViewDesc& view_desc, const DX12Resource& res)
{
    D3D12_RESOURCE_DESC desc = res.default_res->GetDesc();
    bool arrayed = desc.DepthOrArraySize > 1;
    bool ms = desc.SampleDesc.Count > 1;

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
    dsv_desc.Format = DepthStencilFromTypeless(desc.Format);
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
        dsv_desc.Texture2DArray.ArraySize = desc.DepthOrArraySize;
    }
    else if (!arrayed && ms)
    {
        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
    }
    else if (arrayed && ms)
    {
        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
        dsv_desc.Texture2DMSArray.FirstArraySlice = 0;
        dsv_desc.Texture2DMSArray.ArraySize = desc.DepthOrArraySize;
    }
    return dsv_desc;
}
