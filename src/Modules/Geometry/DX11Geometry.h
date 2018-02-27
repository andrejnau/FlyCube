#pragma once

#include "Geometry/Geometry.h"
#include <Utilities/DXUtility.h>
#include <Texture/DX11TextureLoader.h>
#include <Context/DX11Context.h>
#include <d3d11.h>
#include <wrl.h>
#include <glm/glm.hpp>
#include <gli/gli.hpp>
#include <SOIL.h>
#include <map>
#include <vector>

using namespace Microsoft::WRL;

class IAVertexBuffer
{
public:
    template<typename T>
    IAVertexBuffer(Context& context, const std::vector<T>& v)
        : m_context(context)
        , m_stride(sizeof(v.front()))
        , m_offset(0)
        , m_size(v.size() * sizeof(v.front()))
    {
        m_buffer = m_context.CreateBuffer(BindFlag::kVbv, v.size() * sizeof(v.front()), 0, "IAVertexBuffer");
        if (m_buffer)
            m_context.UpdateSubresource(m_buffer, 0, v.data(), 0, 0);
    }

    void BindToSlot(UINT slot)
    {
        if (m_buffer)
            m_context.IASetVertexBuffer(slot, m_buffer, m_size, m_stride);
    }

private:
    Context& m_context;
    ComPtr<IUnknown> m_buffer;
    UINT m_stride;
    UINT m_offset;
    UINT m_size;
};

class IAIndexBuffer
{
public:
    template<typename T>
    IAIndexBuffer(Context& context, const std::vector<T>& v, DXGI_FORMAT format)
        : m_context(context)
        , m_format(format)
        , m_offset(0)
        , m_size(v.size() * sizeof(v.front()))
    {
        m_buffer = m_context.CreateBuffer(BindFlag::kIbv, m_size, 0, "IAIndexBuffer");
        if (m_buffer)
            m_context.UpdateSubresource(m_buffer, 0, v.data(), 0, 0);
    }

    void Bind()
    {
        if (m_buffer)
            m_context.IASetIndexBuffer(m_buffer, m_size, m_format);
    }

private:
    Context& m_context;
    ComPtr<IUnknown> m_buffer;
    UINT m_offset;
    UINT m_size;
    DXGI_FORMAT m_format;
};

class DX11Mesh : public IMesh
{
public:

    DX11Mesh(DX11Context& context, const IMesh& mesh)
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
            assert(m_type2id.count(textures[i].type) == 0);
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

    ComPtr<IUnknown> GetTexture(aiTextureType type)
    {
        auto it = m_type2id.find(type);
        if (it != m_type2id.end())
        {
            return m_tex_srv[it->second];
        }
        return nullptr;
    }

private:
    void InitTextures()
    {
        static std::map<std::string, ComPtr<IUnknown>> cache;

        for (size_t i = 0; i < textures.size(); ++i)
        {
            auto it = cache.find(textures[i].path);
            if (it != cache.end())
            {
                m_tex_srv[i] = it->second;
                continue;
            }

            cache[textures[i].path] = m_tex_srv[i] = CreateTexture(m_context, textures[i]);
        }
    }

private:
    DX11Context& m_context;
    std::map<aiTextureType, size_t> m_type2id;
    std::vector<ComPtr<IUnknown>> m_tex_srv;
};
