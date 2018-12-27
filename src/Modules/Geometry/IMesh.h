#pragma once

#include <string>
#include <vector>
#include <Texture/TextureInfo.h>
#include <glm/glm.hpp>

struct IMesh
{
    struct Material
    {
        std::string name;
    } material;

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;
    std::vector<glm::vec3> tangents;
    std::vector<uint32_t> bones_offset;
    std::vector<uint32_t> bones_count;
    std::vector<uint32_t> indices;
    std::vector<TextureInfo> textures;
};
