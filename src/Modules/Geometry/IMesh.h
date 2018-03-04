#pragma once

#include <string>
#include <vector>
#include <Texture/TextureInfo.h>
#include <glm/glm.hpp>

struct IMesh
{
    struct Material
    {
        glm::vec3 amb = glm::vec3(0.0, 0.0, 0.0);
        glm::vec3 dif = glm::vec3(1.0, 1.0, 1.0);
        glm::vec3 spec = glm::vec3(1.0, 1.0, 1.0);
        float shininess = 32.0;
        std::string name;
    } material;

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;
    std::vector<glm::vec3> tangents;
    std::vector<glm::vec4> colors;
    std::vector<uint32_t> indices;
    std::vector<uint32_t> bones_offset;
    std::vector<uint32_t> bones_count;
    std::vector<TextureInfo> textures;
};
