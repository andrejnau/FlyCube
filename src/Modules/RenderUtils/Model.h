#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>

struct Mesh {
    glm::mat4 matrix = glm::mat4(1.0);

    std::vector<uint32_t> indices;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> tangents;
    std::vector<glm::vec2> texcoords;

    struct {
        std::string base_color;
        std::string normal;
        std::string metallic_roughness;
        std::string metallic;
        std::string roughness;
        std::string smoothness;
        std::string ambient_occlusion;
        std::string emissive;
    } textures;
};

struct Model {
    std::vector<Mesh> meshes;
};
