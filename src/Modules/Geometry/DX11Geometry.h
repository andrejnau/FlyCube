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

class IABuffer
{
protected:
    IABuffer(Context& context)
        : m_context(context)
    {}

    template<typename T>
    ComPtr<ID3D11Buffer> CreateBuffer(const std::vector<T>& v, UINT BindFlags)
    {
        ComPtr<ID3D11Buffer> buffer;
        if (v.empty())
            return buffer;

        D3D11_BUFFER_DESC buffer_desc = {};
        buffer_desc.Usage = D3D11_USAGE_DEFAULT;
        buffer_desc.ByteWidth = static_cast<UINT>(v.size() * sizeof(v.front()));
        buffer_desc.BindFlags = BindFlags;

        D3D11_SUBRESOURCE_DATA buffer_data = {};
        buffer_data.pSysMem = v.data();
        ASSERT_SUCCEEDED(m_context.device->CreateBuffer(&buffer_desc, &buffer_data, &buffer));
        return buffer;
    }

    Context& m_context;
};

class IAVertexBuffer : IABuffer
{
public:
    template<typename T>
    IAVertexBuffer(Context& context, const std::vector<T>& v)
        : IABuffer(context)
        , m_buffer(CreateBuffer(v, D3D11_BIND_VERTEX_BUFFER))
        , m_stride(sizeof(v.front()))
        , m_offset(0)
    {
    }

    void BindToSlot(UINT slot)
    {
        m_context.device_context->IASetVertexBuffers(slot, 1, m_buffer.GetAddressOf(), &m_stride, &m_offset);
    }
private:
    ComPtr<ID3D11Buffer> m_buffer;
    UINT m_stride;
    UINT m_offset;
};

class IAIndexBuffer : IABuffer
{
public:
    template<typename T>
    IAIndexBuffer(Context& context, const std::vector<T>& v, DXGI_FORMAT format)
        : IABuffer(context)
        , m_buffer(CreateBuffer(v, D3D11_BIND_INDEX_BUFFER))
        , m_format(format)
        , m_offset(0)
    {
    }

    void Bind()
    {
        m_context.device_context->IASetIndexBuffer(m_buffer.Get(), m_format, m_offset);
    }

private:
    ComPtr<ID3D11Buffer> m_buffer;
    UINT m_offset;
    DXGI_FORMAT m_format;
};

class DX11Mesh : public IMesh
{
public:

    DX11Mesh(Context& context, const IMesh& mesh)
        : IMesh(mesh)
        , m_context(context)
        , positions_buffer(context, positions)
        , normals_buffer(context, normals)
        , texcoords_buffer(context, texcoords)
        , tangents_buffer(context, tangents)
        , colors_buffer(context, colors)
        , bones_offset_buffer(context, bones_offset)
        , bones_count_buffer(context, bones_count)
        , indices_buffer(context, indices, DXGI_FORMAT_R32_UINT)
    {
        m_tex_srv.resize(textures.size());
        InitTextures();

        for (size_t i = 0; i < textures.size(); ++i)
        {
            m_type2id[textures[i].type] = i;
        }
    }

    IAVertexBuffer positions_buffer;
    IAVertexBuffer normals_buffer;
    IAVertexBuffer texcoords_buffer;
    IAVertexBuffer tangents_buffer;
    IAVertexBuffer colors_buffer;
    IAVertexBuffer bones_offset_buffer;
    IAVertexBuffer bones_count_buffer;
    IAIndexBuffer indices_buffer;

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

private:
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

private:
    Context& m_context;
    std::map<aiTextureType, size_t> m_type2id;
    std::vector<ComPtr<ID3D11ShaderResourceView>> m_tex_srv;
};
