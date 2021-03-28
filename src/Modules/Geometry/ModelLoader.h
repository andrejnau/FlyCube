#pragma once

#include "Geometry/Mesh.h"
#include "Geometry/IModel.h"
#include "Geometry/Bones.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>

class ModelLoader
{
public:
    ModelLoader(const std::string& path, aiPostProcessSteps flags, IModel& model);

private:
    void LoadModel(aiPostProcessSteps flags);
    std::string SplitFilename(const std::string& str);
    void ProcessNode(aiNode* node, const aiScene* scene);
    void ProcessMesh(aiMesh* mesh, const aiScene* scene);
    void FindSimilarTextures(const std::string& mat_name, std::vector<TextureInfo>& textures);
    void LoadMaterialTextures(aiMaterial* mat, aiTextureType aitype, TextureAssetsType type, std::vector<TextureInfo>& textures);

private:
    std::string m_path;
    std::string m_directory;
    Assimp::Importer m_import;
    std::vector<IMesh> m_meshes;
    IModel& m_model;
};