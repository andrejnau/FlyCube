#pragma once

#include <string>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>

struct IMesh
{
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoords;
        glm::vec3 tangent;
        glm::vec3 bitangent;
    };

    using Index = uint32_t;

    struct Texture
    {
        aiTextureType type;
        std::string path;
    };

    struct Material
    {
        glm::vec3 amb = glm::vec3(0.0, 0.0, 0.0);
        glm::vec3 dif = glm::vec3(1.0, 1.0, 1.0);
        glm::vec3 spec = glm::vec3(1.0, 1.0, 1.0);
        float shininess = 32.0;
        aiString name;
    };

    Material material;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<Texture> textures;
};

struct BoundBox
{
    BoundBox()
    {
        x_min = y_min = z_min = std::numeric_limits<float>::max();
        x_max = y_max = z_max = std::numeric_limits<float>::min();
    }

    float x_min, y_min, z_min;
    float x_max, y_max, z_max;
};

struct IModel
{
    virtual IMesh& GetNextMesh() = 0;
    virtual BoundBox& GetBoundBox() = 0;
    virtual const BoundBox& GetBoundBox() const = 0;
};

class ModelLoader
{
public:
    ModelLoader(const std::string& file, IModel& meshes);

private:
    std::string SplitFilename(const std::string& str);
    void LoadModel();
    void ProcessNode(aiNode* node, const aiScene* scene);
    void ProcessMesh(aiMesh* mesh, const aiScene* scene);
    void FindSimilarTextures(std::vector<IMesh::Texture>& textures);
    void LoadMaterialTextures(aiMaterial* mat, aiTextureType type, std::vector<IMesh::Texture>& textures);

private:
    std::string m_path;
    std::string m_directory;
    IModel& m_model;
};

template<typename Mesh>
struct Model : IModel
{
    Model(const std::string& file)
        : m_model_loader(file, *this)
    {
    }

    virtual IMesh& GetNextMesh() override
    {
        meshes.resize(meshes.size() + 1);
        return meshes.back();
    }

    virtual BoundBox& GetBoundBox() override
    {
        return bound_box;
    }

    virtual const BoundBox& GetBoundBox() const override
    {
        return bound_box;
    }

    BoundBox bound_box;
    std::vector<Mesh> meshes;

private:
    ModelLoader m_model_loader;
};

struct ModelSize
{
    ModelSize(const IModel& model)
    {
        const BoundBox& bound_box = model.GetBoundBox();
        z_width = (bound_box.z_max - bound_box.z_min);
        y_width = (bound_box.y_max - bound_box.y_min);
        x_width = (bound_box.x_max - bound_box.x_min);
        model_width = std::max({ z_width, y_width, x_width });
        scale = 1.0 / model_width;
        model_width *= scale;
        offset_x = (bound_box.x_max + bound_box.x_min) / 2.0f;
        offset_y = (bound_box.y_max + bound_box.y_min) / 2.0f;
        offset_z = (bound_box.z_max + bound_box.z_min) / 2.0f;
    }

    float z_width;
    float y_width;
    float x_width;
    float model_width;
    float scale;
    float offset_x;
    float offset_y;
    float offset_z;
};
