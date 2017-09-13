#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <glm/glm.hpp>
#include <Utility.h>
#include <Geometry/Geometry.h>
#include <gli/gli.hpp>
#include <SOIL.h>

using namespace Microsoft::WRL;

struct DX11Mesh : IMesh
{
    ComPtr<ID3D11Buffer> vertBuffer;
    ComPtr<ID3D11Buffer> indexBuffer;

    std::vector<ComPtr<ID3D11ShaderResourceView>> texResources;

    ComPtr<ID3D11ShaderResourceView> CreateSRVFromFile(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& device_context, Texture& texture)
    {
        int width;
        int height;
        unsigned char *image = SOIL_load_image(texture.path.c_str(), &width, &height, 0, SOIL_LOAD_RGBA);

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

    ComPtr<ID3D11ShaderResourceView> CreateSRVFromFileDDS(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& device_context, Texture& texture)
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

        ComPtr<ID3D11Texture2D> resource;
        ASSERT_SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &resource));

        for (std::size_t Level = 0; Level < desc.MipLevels; ++Level)
        {
            size_t num_bytes;
            size_t row_bytes;
            GetSurfaceInfo(Texture.extent(Level).x, Texture.extent(Level).y, desc.Format, &num_bytes, &row_bytes, nullptr);

            D3D11_SUBRESOURCE_DATA textureBufferData;
            ZeroMemory(&textureBufferData, sizeof(textureBufferData));
            textureBufferData.pSysMem = Texture.data(0, 0, Level);
            textureBufferData.SysMemPitch = row_bytes;
            textureBufferData.SysMemSlicePitch = num_bytes;
            device_context->UpdateSubresource(resource.Get(), Level, nullptr, textureBufferData.pSysMem, textureBufferData.SysMemPitch, textureBufferData.SysMemSlicePitch);
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;

        ComPtr<ID3D11ShaderResourceView> srv;
        ASSERT_SUCCEEDED(device->CreateShaderResourceView(resource.Get(), &srvDesc, &srv));
        return srv;
    }

    void InitTextures(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& device_context)
    {
        static std::map<std::string, ComPtr<ID3D11ShaderResourceView>> cache;

        for (size_t i = 0; i < textures.size(); ++i)
        {
            auto it = cache.find(textures[i].path);
            if (it != cache.end())
            {
                texResources[i] = it->second;
                continue;
            }

            if (textures[i].path.find(".dds") != -1)
                texResources[i] = CreateSRVFromFileDDS(device, device_context, textures[i]);
            else
                texResources[i] = CreateSRVFromFile(device, device_context, textures[i]);

            cache[textures[i].path] = texResources[i];
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

        InitTextures(d3d11Device, d3d11DevCon);
    }
};
