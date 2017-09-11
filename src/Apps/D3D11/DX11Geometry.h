#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <glm/glm.hpp>
#include <Utility.h>
#include <Geometry/Geometry.h>
#include <gli/gli.hpp>

using namespace Microsoft::WRL;

struct DX11Mesh : IMesh
{
    ComPtr<ID3D11Buffer> vertBuffer;
    ComPtr<ID3D11Buffer> indexBuffer;

    std::vector<ComPtr<ID3D11ShaderResourceView>> texResources;

    void InitTextures(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& device_context)
    {
        for (size_t i = 0; i < textures.size(); ++i)
        {
            gli::texture Texture = gli::load(textures[i].path);
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

        InitTextures(d3d11Device, d3d11DevCon);
    }
};
