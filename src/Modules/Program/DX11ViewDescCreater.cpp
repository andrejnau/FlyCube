#include "DX11ViewDescCreater.h"

D3D11_SHADER_RESOURCE_VIEW_DESC DX11GeSRVDesc(const D3D11_SHADER_INPUT_BIND_DESC& binding_desc, const ViewDesc& view_desc, const ComPtr<ID3D11Resource>& resource)
{
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    if (binding_desc.Dimension == D3D_SRV_DIMENSION_BUFFER)
    {
        ComPtr<ID3D11Buffer> buffer;
        resource.As(&buffer);
        D3D11_BUFFER_DESC desc = {};
        buffer->GetDesc(&desc);
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.Buffer.FirstElement = 0;
        srv_desc.Buffer.NumElements = desc.ByteWidth / desc.StructureByteStride;
    }
    else
    {
        ComPtr<ID3D11Texture2D> texture;
        resource.As(&texture);
        D3D11_TEXTURE2D_DESC desc = {};
        texture->GetDesc(&desc);

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

        auto setup_mips = [&](UINT& MostDetailedMip, UINT& MipLevels)
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
            srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
            setup_mips(srv_desc.Texture1D.MostDetailedMip, srv_desc.Texture1D.MipLevels);
            break;
        }
        case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
        {
            srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
            setup_mips(srv_desc.Texture1DArray.MostDetailedMip, srv_desc.Texture1DArray.MipLevels);
            srv_desc.Texture1DArray.FirstArraySlice = 0;
            srv_desc.Texture1DArray.ArraySize = desc.ArraySize;
            break;
        }
        case D3D_SRV_DIMENSION_TEXTURE2D:
        {
            srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            setup_mips(srv_desc.Texture2D.MostDetailedMip, srv_desc.Texture2D.MipLevels);
            break;
        }
        case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
        {
            srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
            setup_mips(srv_desc.Texture2DArray.MostDetailedMip, srv_desc.Texture2DArray.MipLevels);
            srv_desc.Texture2DArray.FirstArraySlice = 0;
            srv_desc.Texture2DArray.ArraySize = desc.ArraySize;
            break;
        }
        case D3D_SRV_DIMENSION_TEXTURE2DMS:
        {
            srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
            break;
        }
        case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
        {
            srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
            srv_desc.Texture2DMSArray.FirstArraySlice = 0;
            srv_desc.Texture2DMSArray.ArraySize = desc.ArraySize;
            break;
        }
        case D3D_SRV_DIMENSION_TEXTURE3D:
        {
            srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
            setup_mips(srv_desc.Texture3D.MostDetailedMip, srv_desc.Texture3D.MipLevels);
            break;
        }
        case D3D_SRV_DIMENSION_TEXTURECUBE:
        {
            srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
            setup_mips(srv_desc.TextureCube.MostDetailedMip, srv_desc.TextureCube.MipLevels);
            break;
        }
        case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
        {
            srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
            setup_mips(srv_desc.TextureCubeArray.MostDetailedMip, srv_desc.TextureCubeArray.MipLevels);
            srv_desc.TextureCubeArray.First2DArrayFace = 0;
            srv_desc.TextureCubeArray.NumCubes = desc.ArraySize / 6;
            break;
        }
        default:
            assert(false);
            break;
        }
    }
    return srv_desc;
}

D3D11_UNORDERED_ACCESS_VIEW_DESC DX11GetUAVDesc(const D3D11_SHADER_INPUT_BIND_DESC& binding_desc, const ViewDesc& view_desc, const ComPtr<ID3D11Resource>& resource)
{
    D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
    if (binding_desc.Dimension == D3D_SRV_DIMENSION_BUFFER)
    {
        ComPtr<ID3D11Buffer> buffer;
        resource.As(&buffer);
        D3D11_BUFFER_DESC desc = {};
        buffer->GetDesc(&desc);
        uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        uav_desc.Buffer.FirstElement = 0;
        uav_desc.Buffer.NumElements = desc.ByteWidth / desc.StructureByteStride;
        uav_desc.Buffer.Flags = 0;
    }
    else
    {
        ComPtr<ID3D11Texture2D> texture;
        resource.As(&texture);
        D3D11_TEXTURE2D_DESC desc = {};
        texture->GetDesc(&desc);
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
            uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
            uav_desc.Texture1D.MipSlice = view_desc.level;
            break;
        }
        case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
        {
            uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1DARRAY;
            uav_desc.Texture1DArray.MipSlice = view_desc.level;
            uav_desc.Texture1DArray.FirstArraySlice = 0;
            uav_desc.Texture1DArray.ArraySize = desc.ArraySize;
            break;
        }
        case D3D_SRV_DIMENSION_TEXTURE2D:
        {
            uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
            uav_desc.Texture2D.MipSlice = view_desc.level;
            break;
        }
        case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
        {
            uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
            uav_desc.Texture2DArray.MipSlice = view_desc.level;
            uav_desc.Texture2DArray.FirstArraySlice = 0;
            uav_desc.Texture2DArray.ArraySize = desc.ArraySize;
            break;
        }
        case D3D_SRV_DIMENSION_TEXTURE3D:
        {
            uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
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

D3D11_RENDER_TARGET_VIEW_DESC DX11GetRTVDesc(const D3D11_SIGNATURE_PARAMETER_DESC& binding_desc, const ViewDesc& view_desc, const ComPtr<ID3D11Resource>& resource)
{
    ComPtr<ID3D11Texture2D> texture;
    resource.As(&texture);
    D3D11_TEXTURE2D_DESC desc = {};
    texture->GetDesc(&desc);
    bool arrayed = desc.ArraySize > 1;
    bool ms = desc.SampleDesc.Count > 1;

    D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};
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
        rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        rtv_desc.Texture2D.MipSlice = view_desc.level;
    }
    else if (arrayed && !ms)
    {
        rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        rtv_desc.Texture2DArray.MipSlice = view_desc.level;
        rtv_desc.Texture2DArray.FirstArraySlice = 0;
        rtv_desc.Texture2DArray.ArraySize = desc.ArraySize;
    }
    else if (!arrayed && ms)
    {
        rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
    }
    else if (arrayed && ms)
    {
        rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
        rtv_desc.Texture2DMSArray.FirstArraySlice = 0;
        rtv_desc.Texture2DMSArray.ArraySize = desc.ArraySize;
    }
    return rtv_desc;
}

D3D11_DEPTH_STENCIL_VIEW_DESC DX11GetDSVDesc(const ViewDesc& view_desc, const ComPtr<ID3D11Resource>& resource)
{
    ComPtr<ID3D11Texture2D> texture;
    resource.As(&texture);
    D3D11_TEXTURE2D_DESC desc = {};
    texture->GetDesc(&desc);
    bool arrayed = desc.ArraySize > 1;
    bool ms = desc.SampleDesc.Count > 1;

    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
    dsv_desc.Format = DepthStencilFromTypeless(desc.Format);
    if (!arrayed && !ms)
    {
        dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsv_desc.Texture2D.MipSlice = view_desc.level;
    }
    else if (arrayed && !ms)
    {
        dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
        dsv_desc.Texture2DArray.MipSlice = view_desc.level;
        dsv_desc.Texture2DArray.FirstArraySlice = 0;
        dsv_desc.Texture2DArray.ArraySize = desc.ArraySize;
    }
    else if (!arrayed && ms)
    {
        dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
    }
    else if (arrayed && ms)
    {
        dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
        dsv_desc.Texture2DMSArray.FirstArraySlice = 0;
        dsv_desc.Texture2DMSArray.ArraySize = desc.ArraySize;
    }
    return dsv_desc;
}
