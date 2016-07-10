#pragma once

#include <utilities.h>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct Mesh
{
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoords;
        glm::vec3 tangent;
        glm::vec3 bitangent;
    };

    struct Texture
    {
        GLuint id;
        aiTextureType type;
        aiString path;
    };

    struct Material
    {
        glm::vec3 amb = glm::vec3(0.0);
        glm::vec3 dif = glm::vec3(1.0);
        glm::vec3 spec = glm::vec3(1.0);
        float shininess = 32.0;
        aiString name;
    } material;

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Texture> textures;

    GLuint VAO, VBO, EBO;

    void setupMesh();

    void bindMesh();

    void unbindMesh();
};

class Model
{
public:
    Model(const std::string & file);

    std::string m_path;
    std::string m_directory;
    std::vector<Mesh> meshes;

private:
    std::string splitFilename(const std::string& str);
    void loadModel();

    void processNode(aiNode* node, const aiScene* scene);

    glm::vec4 aiColor4DToVec4(const aiColor4D& x);

    Mesh processMesh(aiMesh* mesh, const aiScene* scene);

    void loadMaterialTextures(Mesh &retMeh, aiMaterial* mat, aiTextureType type);

    void TextureFromFile(Mesh::Texture &texture);
};