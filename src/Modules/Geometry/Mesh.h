#pragma once

#include "Geometry/IMesh.h"
#include "Geometry/IABuffer.h"

class Mesh : public IMesh
{
public:
    Mesh(Context& context, const IMesh& mesh);

    IAVertexBuffer positions_buffer;
    IAVertexBuffer normals_buffer;
    IAVertexBuffer texcoords_buffer;
    IAVertexBuffer tangents_buffer;
    IAVertexBuffer colors_buffer;
    IAVertexBuffer bones_offset_buffer;
    IAVertexBuffer bones_count_buffer;
    IAIndexBuffer indices_buffer;

    Resource::Ptr GetTexture(aiTextureType type);

private:
    void InitTextures();

    Context& m_context;
    std::map<aiTextureType, size_t> m_type2id;
    std::vector<Resource::Ptr> m_tex_srv;
};
