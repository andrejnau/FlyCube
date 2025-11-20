#include "RenderUtils/ModelLoader.h"

#include "Utilities/Asset.h"

#include <assimp/Importer.hpp>
#include <assimp/MemoryIOWrapper.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/gtc/type_ptr.hpp>

#include <cassert>

namespace {

class MyIOSystem : public Assimp::IOSystem {
public:
    bool Exists(const char* pFile) const override
    {
        return AssetFileExists(pFile);
    }

    char getOsSeparator() const override
    {
        return '/';
    }

    Assimp::IOStream* Open(const char* pFile, const char* pMode) override
    {
        auto file = AssetLoadBinaryFile(pFile);
        uint8_t* buffer = new uint8_t[file.size()];
        memcpy(buffer, file.data(), file.size());
        return new Assimp::MemoryIOStream(buffer, file.size(), true);
    }

    void Close(Assimp::IOStream* pFile) override
    {
        delete pFile;
    }
};

glm::vec3 ToVec3(const aiVector3D& vector)
{
    return glm::vec3(vector.x, vector.y, vector.z);
}

class ModelLoader {
public:
    ModelLoader(const std::string& path, Model& model)
        : path_(path)
    {
        importer_.SetIOHandler(new MyIOSystem());
        scene_ =
            importer_.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace |
                                         aiProcess_FlipUVs | aiProcess_FlipWindingOrder | aiProcess_OptimizeMeshes);
        assert(scene_);
        assert(scene_->mRootNode);
        assert(scene_->mFlags != AI_SCENE_FLAGS_INCOMPLETE);
        ProcessScene(model);
    }

private:
    void ProcessScene(Model& model)
    {
        ProcessNode(scene_->mRootNode, glm::mat4(1.0), model);
    }

    void ProcessNode(aiNode* node, glm::mat4 transform, Model& model)
    {
        transform = glm::transpose(glm::make_mat4(&node->mTransformation.a1)) * transform;
        for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
            aiMesh* mesh = scene_->mMeshes[node->mMeshes[i]];
            ProcessMesh(mesh, transform, model);
        }
        for (uint32_t i = 0; i < node->mNumChildren; ++i) {
            ProcessNode(node->mChildren[i], transform, model);
        }
    }

    void ProcessMesh(aiMesh* mesh, const glm::mat4& transform, Model& model)
    {
        Mesh& cur_mesh = model.meshes.emplace_back();
        cur_mesh.matrix = transform;

        for (uint32_t i = 0; i < mesh->mNumFaces; ++i) {
            for (uint32_t j = 0; j < mesh->mFaces[i].mNumIndices; ++j) {
                cur_mesh.indices.push_back(mesh->mFaces[i].mIndices[j]);
            }
        }

        for (uint32_t i = 0; i < mesh->mNumVertices; ++i) {
            auto& position = cur_mesh.positions.emplace_back();
            auto& normal = cur_mesh.normals.emplace_back();
            auto& tangent = cur_mesh.tangents.emplace_back();
            auto& texcoord = cur_mesh.texcoords.emplace_back();

            if (mesh->HasPositions()) {
                position = ToVec3(mesh->mVertices[i]);
            }
            if (mesh->HasNormals()) {
                normal = ToVec3(mesh->mNormals[i]);
            }
            if (mesh->HasTangentsAndBitangents()) {
                tangent = ToVec3(mesh->mTangents[i]);
            }
            if (mesh->HasTextureCoords(/*index=*/0)) {
                texcoord = ToVec3(mesh->mTextureCoords[0][i]);
            }
        }

        if (mesh->mMaterialIndex < scene_->mNumMaterials) {
            aiMaterial* material = scene_->mMaterials[mesh->mMaterialIndex];
            if (path_.ends_with(".gltf")) {
                cur_mesh.textures.base_color = GetTexturePath(material, aiTextureType_BASE_COLOR);
                cur_mesh.textures.normal = GetTexturePath(material, aiTextureType_NORMALS);
                cur_mesh.textures.metallic_roughness = GetTexturePath(material, aiTextureType_GLTF_METALLIC_ROUGHNESS);
                cur_mesh.textures.ambient_occlusion = GetTexturePath(material, aiTextureType_LIGHTMAP);
                cur_mesh.textures.emissive = GetTexturePath(material, aiTextureType_EMISSIVE);
            } else if (path_.ends_with(".obj")) {
                // base_color: map_Kd
                cur_mesh.textures.base_color = GetTexturePath(material, aiTextureType_DIFFUSE);
                // normal: norm
                cur_mesh.textures.normal = GetTexturePath(material, aiTextureType_NORMALS);
                // metallic: map_Pm
                cur_mesh.textures.metallic = GetTexturePath(material, aiTextureType_METALNESS);
                // roughness: map_Pr
                cur_mesh.textures.roughness = GetTexturePath(material, aiTextureType_DIFFUSE_ROUGHNESS);
                // smoothness: map_Ns
                cur_mesh.textures.smoothness = GetTexturePath(material, aiTextureType_SHININESS);
                // ambient_occlusion: map_Ka
                cur_mesh.textures.ambient_occlusion = GetTexturePath(material, aiTextureType_AMBIENT);
                // emissive: map_Ke
                cur_mesh.textures.emissive = GetTexturePath(material, aiTextureType_EMISSIVE);
            }
        }
    }

    std::string GetTexturePath(aiMaterial* material, aiTextureType type)
    {
        aiString texture_path;
        if (material->GetTexture(type, /*index=*/0, &texture_path) != AI_SUCCESS) {
            return {};
        }
        return GetDirectory() + "/" + texture_path.C_Str();
    }

    std::string GetDirectory() const
    {
        return path_.substr(0, path_.find_last_of("/"));
    }

private:
    std::string path_;
    Assimp::Importer importer_;
    const aiScene* scene_ = nullptr;
};

} // namespace

std::unique_ptr<Model> LoadModel(const std::string& path)
{
    std::unique_ptr<Model> model = std::make_unique<Model>();
    ModelLoader loader(path, *model);
    return model;
}
