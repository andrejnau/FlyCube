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
        ranges.back().index_count = static_cast<uint32_t>(mesh.indices.size());
        ranges.back().start_index_location = static_cast<uint32_t>(indices.size());
        ranges.back().base_vertex_location = static_cast<int32_t>(cur_size);

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
    , indices(context, m_data->indices, gli::format::FORMAT_R32_UINT_PACK32)
    , ranges(std::move(m_data->ranges))
{
    m_data.reset();
}

Material::Material(TextureCache& cache, const IMesh::Material& material, std::vector<TextureInfo>& textures)
    : IMesh::Material(material)
{
    for (size_t i = 0; i < textures.size(); ++i)
    {
        auto tex = cache.Load(textures[i].path);
        switch (textures[i].type)
        {
        case aiTextureType_AMBIENT:
            texture.ambient = tex;
            break;
        case aiTextureType_DIFFUSE:
            texture.diffuse = tex;
            texture.albedo = tex;
            break;
        case aiTextureType_SPECULAR:
            texture.specular = tex;
            texture.roughness = tex;
            break;
        case aiTextureType_HEIGHT:
            texture.normal = tex;
            break;
        case aiTextureType_OPACITY:
            texture.alpha = tex;
            break;
        case aiTextureType_SHININESS:
            texture.metalness = tex;
            texture.shininess = tex;
            break;
        case aiTextureType_LIGHTMAP:
            texture.ao = tex;
            break;
        case aiTextureType_EMISSIVE:
            texture.gloss = tex;
            break;
        }
    }

    if (!texture.ambient)
        texture.ambient = cache.CreateTextuteStab(glm::vec4(material.amb, 0.0));
    if (!texture.diffuse)
        texture.diffuse = cache.CreateTextuteStab(glm::vec4(material.dif, 0.0));
    if (!texture.specular)
        texture.specular = cache.CreateTextuteStab(glm::vec4(material.spec, 0.0));
    if (!texture.shininess)
        texture.shininess = cache.CreateTextuteStab(glm::vec4(material.shininess, 0.0, 0.0, 0.0));
    if (!texture.alpha)
        texture.alpha = cache.CreateTextuteStab(glm::vec4(1.0));
    if (!texture.ao)
        texture.ao = cache.CreateTextuteStab(glm::vec4(1.0));
}
