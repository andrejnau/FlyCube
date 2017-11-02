#pragma once

#include "Geometry/Geometry.h"
#include <Utilities/DXUtility.h>
#include <Texture/DX11TextureLoader.h>
#include <d3d11.h>
#include <wrl.h>
#include <glm/glm.hpp>
#include <gli/gli.hpp>
#include <SOIL.h>
#include <map>
#include <vector>

using namespace Microsoft::WRL;

struct DX11Mesh : IMesh
{
    ComPtr<ID3D11Buffer> positions_buffer;
    ComPtr<ID3D11Buffer> normals_buffer;
    ComPtr<ID3D11Buffer> texcoords_buffer;
    ComPtr<ID3D11Buffer> tangents_buffer;
    ComPtr<ID3D11Buffer> indices_buffer;

    std::map<aiTextureType, size_t> m_type2id;
    std::vector<ComPtr<ID3D11ShaderResourceView>> texResources;

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

            cache[textures[i].path] = texResources[i] = CreateTexture(device, device_context, textures[i]);
        }
    }

    template<typename T>
    ComPtr<ID3D11Buffer> CreateBuffer(ComPtr<ID3D11Device>& device, const std::vector<T>& v, UINT BindFlags)
    {
        ComPtr<ID3D11Buffer> buffer;
        D3D11_BUFFER_DESC buffer_desc = {};
        buffer_desc.Usage = D3D11_USAGE_DEFAULT;
        buffer_desc.ByteWidth = v.size() * sizeof(v.front());
        buffer_desc.BindFlags = BindFlags;

        D3D11_SUBRESOURCE_DATA buffer_data = {};
        buffer_data.pSysMem = v.data();
        ASSERT_SUCCEEDED(device->CreateBuffer(&buffer_desc, &buffer_data, &buffer));
        return buffer;
    }

    void SetupMesh(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& device_context)
    {
        positions_buffer = CreateBuffer(device, positions, D3D11_BIND_VERTEX_BUFFER);
        normals_buffer = CreateBuffer(device, normals, D3D11_BIND_VERTEX_BUFFER);
        texcoords_buffer = CreateBuffer(device, texcoords, D3D11_BIND_VERTEX_BUFFER);
        tangents_buffer = CreateBuffer(device, tangents, D3D11_BIND_VERTEX_BUFFER);
        indices_buffer = CreateBuffer(device, indices, D3D11_BIND_INDEX_BUFFER);

        texResources.resize(textures.size());
        InitTextures(device, device_context);

        for (size_t i = 0; i < textures.size(); ++i)
        {
            m_type2id[textures[i].type] = i;
        }
    }

    void UnsetTexture(ComPtr<ID3D11DeviceContext>& device_context, UINT slot)
    {
        ID3D11ShaderResourceView* srv = nullptr;
        device_context->PSSetShaderResources(slot, 1, &srv);
    }

    void SetTexture(ComPtr<ID3D11DeviceContext>& device_context, aiTextureType type, UINT slot)
    {
        ID3D11ShaderResourceView* srv = nullptr;
        auto it = m_type2id.find(type);
        if (it != m_type2id.end())
            srv = texResources[it->second].Get();
        device_context->PSSetShaderResources(slot, 1, &srv);
    }

    void SetVertexBuffer(ComPtr<ID3D11DeviceContext>& device_context, UINT slot, ComPtr<ID3D11Buffer>& buffer, UINT stride, UINT offset)
    {
        device_context->IASetVertexBuffers(slot, 1, buffer.GetAddressOf(), &stride, &offset);
    }
};
