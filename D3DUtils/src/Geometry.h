#pragma once

#include <string>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct IMesh
{
    struct IVertex
    {
        aiVector3D position;
        aiVector3D normal;
        aiVector2D texCoords;
        aiVector3D tangent;
        aiVector3D bitangent;
    };

    using IIndex = uint32_t;

    struct ITexture
    {
        aiTextureType type;
        std::string path;
    };

    struct IMaterial
    {
        aiVector3D amb = aiVector3D(0.0, 0.0, 0.0);
        aiVector3D dif = aiVector3D(1.0, 1.0, 1.0);
        aiVector3D spec = aiVector3D(1.0, 1.0, 1.0);
        float shininess = 32.0;
        aiString name;
    };

    virtual void AddVertex(const IVertex&) = 0;
    virtual void AddIndex(const IIndex&) = 0;
    virtual void AddTexture(const ITexture&) = 0;
    virtual void SetMaterial(const IMaterial&) = 0;
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
};

class ModelParser
{
public:
    ModelParser(const std::string& file, IModel& meshes);

private:
    std::string SplitFilename(const std::string& str);
    void LoadModel();
    void ProcessNode(aiNode* node, const aiScene* scene);
    void ProcessMesh(aiMesh* mesh, const aiScene* scene);
    void FindSimilarTextures(std::vector<IMesh::ITexture>& textures);
    void LoadMaterialTextures(aiMaterial* mat, aiTextureType type, std::vector<IMesh::ITexture>& textures);

private:
    std::string m_path;
    std::string m_directory;
    IModel& m_model;
};

template<typename Mesh>
struct Model : IModel
{
    Model(const std::string& file)
        : model_parser(file, *this)
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

    BoundBox bound_box;
    std::vector<Mesh> meshes;

private:
    ModelParser model_parser;
};