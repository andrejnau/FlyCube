#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <glm/glm.hpp>
#include <Utility.h>
#include <Geometry/Geometry.h>
#include <SOIL.h>

using namespace Microsoft::WRL;

struct DX11Mesh : IMesh
{
    ComPtr<ID3D11Buffer> vertBuffer;
    ComPtr<ID3D11Buffer> indexBuffer;

    std::vector<ComPtr<ID3D11ShaderResourceView>> texResources;

    struct TexInfo
    {
        std::vector<uint8_t> image;
        int width;
        int height;
        int num_bits_per_pixel;
        int image_size;
        int bytes_per_row;
    };

    TexInfo LoadImageDataFromFile(const std::string & filename)
    {
        TexInfo texInfo = {};

        unsigned char *image = SOIL_load_image(filename.c_str(), &texInfo.width, &texInfo.height, 0, SOIL_LOAD_RGBA);

        DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        texInfo.num_bits_per_pixel = BitsPerPixel(dxgiFormat); // number of bits per pixel
        texInfo.bytes_per_row = (texInfo.width * texInfo.num_bits_per_pixel) / 8; // number of bytes in each row of the image data
        texInfo.image_size = texInfo.bytes_per_row * texInfo.height; // total image size in bytes

        texInfo.image.resize(texInfo.image_size);
        memcpy(texInfo.image.data(), image, texInfo.image_size);
        SOIL_free_image_data(image);

        return texInfo;
    }

    void InitTextures(ComPtr<ID3D11Device>& device)
    {
        for (size_t i = 0; i < textures.size(); ++i)
        {
            auto metadata = LoadImageDataFromFile(textures[i].path);

            ComPtr<ID3D11Texture2D> resource;

            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width = static_cast<UINT>(metadata.width);
            desc.Height = static_cast<UINT>(metadata.height);
            desc.MipLevels = static_cast<UINT>(1);
            desc.ArraySize = static_cast<UINT>(1);
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = 0;

            D3D11_SUBRESOURCE_DATA textureBufferData;
            ZeroMemory(&textureBufferData, sizeof(textureBufferData));
            textureBufferData.pSysMem = metadata.image.data();
            textureBufferData.SysMemPitch = metadata.bytes_per_row; // size of all our triangle vertex data
            textureBufferData.SysMemSlicePitch = metadata.bytes_per_row * metadata.height; // also the size of our triangle vertex data

            ASSERT_SUCCEEDED(device->CreateTexture2D(&desc, &textureBufferData, &resource));

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;

            ASSERT_SUCCEEDED(device->CreateShaderResourceView(resource.Get(), &srvDesc, &texResources[i]));
        }
    }

    void SetupMesh(ComPtr<ID3D11Device>& d3d11Device, ComPtr<ID3D11DeviceContext>& d3d11DevCon)
    {
        texResources.resize(textures.size());
        D3D11_BUFFER_DESC vertexBufferDesc;
        ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

        vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        vertexBufferDesc.ByteWidth = vertices.size() * sizeof(vertices[0]);
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vertexBufferDesc.CPUAccessFlags = 0;
        vertexBufferDesc.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA vertexBufferData;

        ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
        vertexBufferData.pSysMem = vertices.data();
        HRESULT hr = d3d11Device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &vertBuffer);

        D3D11_BUFFER_DESC indexBufferDesc;
        ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));
        indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        indexBufferDesc.ByteWidth = indices.size() * sizeof(indices[0]);
        indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        indexBufferDesc.CPUAccessFlags = 0;
        indexBufferDesc.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA iinitData;
        iinitData.pSysMem = indices.data();
        hr = d3d11Device->CreateBuffer(&indexBufferDesc, &iinitData, &indexBuffer);

        InitTextures(d3d11Device);
    }
};
