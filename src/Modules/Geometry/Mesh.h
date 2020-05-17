#pragma once

#include <memory>
#include "Geometry/IMesh.h"
#include "Geometry/IABuffer.h"
#include <Texture/TextureLoader.h>
#include <Texture/TextureCache.h>

struct MeshRange
{
    size_t id = 0;
    uint32_t index_count = 0;
    uint32_t start_index_location = 0;
    int32_t base_vertex_location = 0;
};

class MergedMesh
{
public:
    MergedMesh(const std::vector<IMesh>& meshes);
    
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;
    std::vector<glm::vec3> tangents;
    std::vector<uint32_t> bones_offset;
    std::vector<uint32_t> bones_count;
    std::vector<uint32_t> indices;
    std::vector<MeshRange> ranges;
};

class Material : public IMesh::Material
{
public:
    Material(TextureCache& cache, const IMesh::Material& material, std::vector<TextureInfo>& textures);

    struct
    {
        std::shared_ptr<Resource> albedo;
        std::shared_ptr<Resource> normal;
        std::shared_ptr<Resource> roughness;
        std::shared_ptr<Resource> glossiness;
        std::shared_ptr<Resource> metalness;
        std::shared_ptr<Resource> occlusion;
        std::shared_ptr<Resource> opacity;
    } texture;
};

class IAMergedMesh
{
    std::unique_ptr<MergedMesh> m_data;
public:
    IAMergedMesh(Context& context, std::vector<IMesh>& meshes);

    IAVertexBuffer positions;
    IAVertexBuffer normals;
    IAVertexBuffer texcoords;
    IAVertexBuffer tangents;
    IAVertexBuffer bones_offset;
    IAVertexBuffer bones_count;
    IAIndexBuffer indices;
    std::vector<MeshRange> ranges;
private:
    std::map<std::string, std::shared_ptr<Resource>> m_tex_cache;
};
