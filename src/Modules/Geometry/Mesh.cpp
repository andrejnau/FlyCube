#include "Geometry/Mesh.h"
#include <Texture/TextureLoader.h>

MergedMesh::MergedMesh(const std::vector<IMesh>& meshes)
{
    size_t count_non_empty_positions = 0;
    size_t count_non_empty_normals = 0;
    size_t count_non_empty_texcoords = 0;
    size_t count_non_empty_tangents = 0;
    size_t count_non_empty_bones_offset = 0;
    size_t count_non_empty_bones_count = 0;

    for (const auto & mesh : meshes)
    {
        count_non_empty_positions += !mesh.positions.empty();
        count_non_empty_normals += !mesh.normals.empty();
        count_non_empty_texcoords += !mesh.texcoords.empty();
        count_non_empty_tangents += !mesh.tangents.empty();
        count_non_empty_bones_offset += !mesh.bones_offset.empty();
        count_non_empty_bones_count += !mesh.bones_count.empty();
    }

    size_t cur_size = 0;
    size_t id = 0;
    for (const auto & mesh : meshes)
    {
        ranges.emplace_back();
        ranges.back().id = id++;
        ranges.back().index_count = mesh.indices.size();
        ranges.back().start_index_location = indices.size();
        ranges.back().base_vertex_location = cur_size;

        size_t max_size = 0;
        max_size = std::max(max_size, mesh.positions.size());
        max_size = std::max(max_size, mesh.normals.size());
        max_size = std::max(max_size, mesh.texcoords.size());
        max_size = std::max(max_size, mesh.tangents.size());
        max_size = std::max(max_size, mesh.bones_offset.size());
        max_size = std::max(max_size, mesh.bones_count.size());

        if (count_non_empty_positions)
        {
            std::copy(mesh.positions.begin(), mesh.positions.end(), back_inserter(positions));
            positions.resize(cur_size + max_size);
        }

        if (count_non_empty_normals)
        {
            std::copy(mesh.normals.begin(), mesh.normals.end(), back_inserter(normals));
            normals.resize(cur_size + max_size);
        }

        if (count_non_empty_texcoords)
        {
            std::copy(mesh.texcoords.begin(), mesh.texcoords.end(), back_inserter(texcoords));
            texcoords.resize(cur_size + max_size);
        }

        if (count_non_empty_tangents)
        {
            std::copy(mesh.tangents.begin(), mesh.tangents.end(), back_inserter(tangents));
            tangents.resize(cur_size + max_size);
        }

        if (count_non_empty_bones_offset)
        {
            std::copy(mesh.bones_offset.begin(), mesh.bones_offset.end(), back_inserter(bones_offset));
            bones_offset.resize(cur_size + max_size);
        }

        if (count_non_empty_bones_count)
        {
            std::copy(mesh.bones_count.begin(), mesh.bones_count.end(), back_inserter(bones_count));
            bones_count.resize(cur_size + max_size);
        }

        std::copy(mesh.indices.begin(), mesh.indices.end(), back_inserter(indices));
        cur_size += max_size;
    }
}

IAMergedMesh::IAMergedMesh(Context & context, std::vector<IMesh>& meshes)
    : m_data(std::make_unique<MergedMesh>(meshes))
    , positions(context, m_data->positions)
    , normals(context, m_data->normals)
    , texcoords(context, m_data->texcoords)
    , tangents(context, m_data->tangents)
    , bones_offset(context, m_data->bones_offset)
    , bones_count(context, m_data->bones_count)
    , indices(context, m_data->indices, DXGI_FORMAT_R32_UINT)
    , ranges(std::move(m_data->ranges))
{
    m_data.reset();
}

Material::Material(Context & context, const IMesh::Material & material, std::vector<TextureInfo>& textures)
    : IMesh::Material(material)
{
    static std::map<std::string, Resource::Ptr> tex_cache;
    for (size_t i = 0; i < textures.size(); ++i)
    {
        auto it = tex_cache.find(textures[i].path);
        if (it == tex_cache.end())
            it = tex_cache.emplace(textures[i].path, CreateTexture(context, textures[i])).first;

        auto& tex = it->second;
        switch (textures[i].type)
        {
        case aiTextureType_AMBIENT:
            texture.ambient = tex;
            break;
        case aiTextureType_DIFFUSE:
            texture.diffuse = tex;
            break;
        case aiTextureType_SPECULAR:
            texture.specular = tex;
            break;
        case aiTextureType_SHININESS:
            texture.shininess = tex;
            break;
        case aiTextureType_HEIGHT:
            texture.normal = tex;
            break;
        case aiTextureType_OPACITY:
            texture.alpha = tex;
            break;
        case aiTextureType_EMISSIVE:
            texture.metalness = tex;
            break;
        case aiTextureType_LIGHTMAP:
            texture.ao = tex;
            break;
        }
    }
}
