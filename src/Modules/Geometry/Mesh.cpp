#include "Geometry/Mesh.h"
#include <Texture/TextureLoader.h>

Mesh::Mesh(Context& context, const IMesh& mesh)
    : IMesh(mesh)
    , m_context(context)
    , positions_buffer(context, positions)
    , normals_buffer(context, normals)
    , texcoords_buffer(context, texcoords)
    , tangents_buffer(context, tangents)
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

Resource::Ptr Mesh::GetTexture(aiTextureType type)
{
    auto it = m_type2id.find(type);
    if (it != m_type2id.end())
    {
        return m_tex_srv[it->second];
    }
    return nullptr;
}

void Mesh::InitTextures()
{
    static std::map<std::string, Resource::Ptr> cache;

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
