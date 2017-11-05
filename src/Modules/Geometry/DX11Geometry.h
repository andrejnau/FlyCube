#pragma once

#include "Geometry/Geometry.h"
#include <Utilities/DXUtility.h>
#include <Texture/DX11TextureLoader.h>
#include <Context/Context.h>
#include <d3d11.h>
#include <wrl.h>
#include <glm/glm.hpp>
#include <gli/gli.hpp>
#include <SOIL.h>
#include <map>
#include <vector>

using namespace Microsoft::WRL;

class DX11Mesh : public IMesh
{
public:
    DX11Mesh(Context& context)
        : m_context(context)
    {
    }

    void SetupMesh()
    {
        m_positions_buffer = CreateBuffer(positions, D3D11_BIND_VERTEX_BUFFER);
        m_normals_buffer = CreateBuffer(normals, D3D11_BIND_VERTEX_BUFFER);
        m_texcoords_buffer = CreateBuffer(texcoords, D3D11_BIND_VERTEX_BUFFER);
        m_tangents_buffer = CreateBuffer(tangents, D3D11_BIND_VERTEX_BUFFER);
        m_colors_buffer = CreateBuffer(colors, D3D11_BIND_VERTEX_BUFFER);
        m_indices_buffer = CreateBuffer(indices, D3D11_BIND_INDEX_BUFFER);

        m_tex_srv.resize(textures.size());
        InitTextures();

        for (size_t i = 0; i < textures.size(); ++i)
        {
            m_type2id[textures[i].type] = i;
        }
    }

    void SetTexture(aiTextureType type, UINT slot)
    {
        ID3D11ShaderResourceView* srv = nullptr;
        auto it = m_type2id.find(type);
        if (it != m_type2id.end())
            srv = m_tex_srv[it->second].Get();
        m_context.device_context->PSSetShaderResources(slot, 1, &srv);
    }

    void UnsetTexture(UINT slot)
    {
        ID3D11ShaderResourceView* srv = nullptr;
        m_context.device_context->PSSetShaderResources(slot, 1, &srv);
    }

    void SetVertexBuffer(UINT slot, VertexType type)
    {
        if (type == VertexType::kPosition)
            SetVertexBufferImpl(slot, m_positions_buffer, sizeof(positions.front()), 0);
        else if (type == VertexType::kTexcoord)
            SetVertexBufferImpl(slot, m_texcoords_buffer, sizeof(texcoords.front()), 0);
        else if (type == VertexType::kNormal)
            SetVertexBufferImpl(slot, m_normals_buffer, sizeof(normals.front()), 0);
        else if (type == VertexType::kTangent)
            SetVertexBufferImpl(slot, m_tangents_buffer, sizeof(tangents.front()), 0);
        else if (type == VertexType::kColor)
            SetVertexBufferImpl(slot, m_colors_buffer, sizeof(colors.front()), 0);
    }

    void SetIndexBuffer()
    {
        m_context.device_context->IASetIndexBuffer(m_indices_buffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    }

private:

    void SetVertexBufferImpl(UINT slot, ComPtr<ID3D11Buffer>& buffer, UINT stride, UINT offset)
    {
        m_context.device_context->IASetVertexBuffers(slot, 1, buffer.GetAddressOf(), &stride, &offset);
    }

    void InitTextures()
    {
        static std::map<std::string, ComPtr<ID3D11ShaderResourceView>> cache;

        for (size_t i = 0; i < textures.size(); ++i)
        {
            auto it = cache.find(textures[i].path);
            if (it != cache.end())
            {
                m_tex_srv[i] = it->second;
                continue;
            }

            cache[textures[i].path] = m_tex_srv[i] = CreateTexture(m_context.device, m_context.device_context, textures[i]);
        }
    }

    template<typename T>
    ComPtr<ID3D11Buffer> CreateBuffer(const std::vector<T>& v, UINT BindFlags)
    {
        ComPtr<ID3D11Buffer> buffer;
        D3D11_BUFFER_DESC buffer_desc = {};
        buffer_desc.Usage = D3D11_USAGE_DEFAULT;
        buffer_desc.ByteWidth = static_cast<UINT>(v.size() * sizeof(v.front()));
        buffer_desc.BindFlags = BindFlags;

        D3D11_SUBRESOURCE_DATA buffer_data = {};
        buffer_data.pSysMem = v.data();
        ASSERT_SUCCEEDED(m_context.device->CreateBuffer(&buffer_desc, &buffer_data, &buffer));
        return buffer;
    }

private:
    Context& m_context;
    ComPtr<ID3D11Buffer> m_positions_buffer;
    ComPtr<ID3D11Buffer> m_normals_buffer;
    ComPtr<ID3D11Buffer> m_texcoords_buffer;
    ComPtr<ID3D11Buffer> m_tangents_buffer;
    ComPtr<ID3D11Buffer> m_colors_buffer;
    ComPtr<ID3D11Buffer> m_indices_buffer;

    std::map<aiTextureType, size_t> m_type2id;
    std::vector<ComPtr<ID3D11ShaderResourceView>> m_tex_srv;
};
