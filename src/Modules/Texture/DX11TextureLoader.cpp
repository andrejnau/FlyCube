#include "Texture/DX11TextureLoader.h"
#include "Texture/DXGIFormatHelper.h"
#include <Utilities/DXUtility.h>
#include <gli/gli.hpp>
#include <SOIL.h>

#include <Context/Context.h>

using namespace Microsoft::WRL;

ComPtr<ID3D11ShaderResourceView> CreateSRVFromFile(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& device_context, TextureInfo& texture)
{
    int width = 0;
    int height = 0;
    unsigned char *image = SOIL_load_image(texture.path.c_str(), &width, &height, 0, SOIL_LOAD_RGBA);
    if (!image)
        return {};

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = static_cast<UINT>(width);
    desc.Height = static_cast<UINT>(height);
    desc.ArraySize = static_cast<UINT>(1);
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    size_t num_bytes;
    size_t row_bytes;
    GetSurfaceInfo(width, height, desc.Format, &num_bytes, &row_bytes, nullptr);

    D3D11_SUBRESOURCE_DATA textureBufferData;
    ZeroMemory(&textureBufferData, sizeof(textureBufferData));
    textureBufferData.pSysMem = image;
    textureBufferData.SysMemPitch = row_bytes;
    textureBufferData.SysMemSlicePitch = num_bytes;

    ComPtr<ID3D11Texture2D> resource;
    ASSERT_SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &resource));
    device_context->UpdateSubresource(resource.Get(), 0, nullptr, textureBufferData.pSysMem, textureBufferData.SysMemPitch, textureBufferData.SysMemSlicePitch);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = -1;

    ComPtr<ID3D11ShaderResourceView> srv;
    ASSERT_SUCCEEDED(device->CreateShaderResourceView(resource.Get(), &srvDesc, &srv));
    device_context->GenerateMips(srv.Get());

    SOIL_free_image_data(image);
    return srv;
}

ComPtr<IUnknown> CreateSRVFromFileDDS(Context& context, TextureInfo& texture)
{
    gli::texture Texture = gli::load(texture.path);
    auto format = gli::dx().translate(Texture.format());

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = static_cast<UINT>(Texture.extent(0).x);
    desc.Height = static_cast<UINT>(Texture.extent(0).y);
    desc.ArraySize = static_cast<UINT>(1);
    desc.Format = (DXGI_FORMAT)format.DXGIFormat.DDS;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MipLevels = Texture.levels();

    ComPtr<IUnknown> resource = context.CreateTexture(BindFlag::kSrv, desc.Format, 1, desc.Width, desc.Height, desc.ArraySize);

    for (std::size_t Level = 0; Level < 1 + 0 * desc.MipLevels; ++Level)
    {
        size_t num_bytes;
        size_t row_bytes;
        GetSurfaceInfo(Texture.extent(Level).x, Texture.extent(Level).y, desc.Format, &num_bytes, &row_bytes, nullptr);

        D3D11_SUBRESOURCE_DATA textureBufferData;
        ZeroMemory(&textureBufferData, sizeof(textureBufferData));
        textureBufferData.pSysMem = Texture.data(0, 0, Level);
        textureBufferData.SysMemPitch = row_bytes;
        textureBufferData.SysMemSlicePitch = num_bytes;
        context.UpdateSubresource(resource, Level, textureBufferData.pSysMem, textureBufferData.SysMemPitch, textureBufferData.SysMemSlicePitch);
    }

    return resource;
}

ComPtr<IUnknown> CreateTexture(Context& context, TextureInfo & texture)
{
    if (texture.path.find(".dds") != -1)
        return CreateSRVFromFileDDS(context, texture);
    return nullptr;
    //else
    //    return CreateSRVFromFile(device, device_context, texture);
}
