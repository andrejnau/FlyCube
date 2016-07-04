#pragma once

#include <Windows.h>
#include <d3dx12.h>
#include <d3d12.h>

#include <vector>
#include <fstream>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <SOIL.h>

#include "Util.h"

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
        uint32_t id;
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
    std::vector<uint32_t> indices;
    std::vector<Texture> textures;

    ID3D12Resource* vertexBuffer; // a default buffer in GPU memory that we will load vertex data for our triangle into

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView; // a structure containing a pointer to the vertex data in gpu memory
                                               // the total size of the buffer, and the size of each element (vertex)

    ID3D12Resource* indexBuffer; // a default buffer in GPU memory that we will load index data for our triangle into

    D3D12_INDEX_BUFFER_VIEW indexBufferView; // a structure holding information about the index buffer

    void setupMesh(CommandHelper commandHelper)
    {
        vertexBuffer = commandHelper.pushBuffer(vertices.data(), vertices.size() * sizeof(vertices[0]));

        // create a vertex buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
        vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
        vertexBufferView.StrideInBytes = sizeof(vertices[0]);
        vertexBufferView.SizeInBytes = vertices.size() * sizeof(vertices[0]);

        indexBuffer = commandHelper.pushBuffer(indices.data(), indices.size() * sizeof(indices[0]));
        // create a vertex buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
        indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
        indexBufferView.Format = DXGI_FORMAT_R32_UINT; // 32-bit unsigned integer (this is what a dword is, double word, a word is 2 bytes)
        indexBufferView.SizeInBytes = indices.size() * sizeof(indices[0]);
    }
};

class Model
{
public:
    Model(const std::string & file)
        : m_path(PROJECT_RESOURCE_DIR + file)
        , m_directory(splitFilename(m_path))
    {
        loadModel();
    }

    std::string m_path;
    std::string m_directory;
    std::vector<Mesh> meshes;

private:
    std::string splitFilename(const std::string& str)
    {
        return str.substr(0, str.find_last_of("/"));
    }
    void loadModel()
    {
        Assimp::Importer import;
        const aiScene* scene = import.ReadFile(m_path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace);
        assert(scene && scene->mFlags != AI_SCENE_FLAGS_INCOMPLETE && scene->mRootNode);
        processNode(scene->mRootNode, scene);
    }

    void processNode(aiNode* node, const aiScene* scene)
    {
        for (uint32_t i = 0; i < node->mNumMeshes; ++i)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }

        for (uint32_t i = 0; i < node->mNumChildren; ++i)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    glm::vec4 aiColor4DToVec4(const aiColor4D& x)
    {
        return glm::vec4(x.r, x.g, x.b, x.a);
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene)
    {
        Mesh retMeh;
        // Walk through each of the mesh's vertices
        for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
        {
            Mesh::Vertex vertex;

            if (mesh->HasPositions())
            {
                vertex.position.x = mesh->mVertices[i].x;
                vertex.position.y = mesh->mVertices[i].y;
                vertex.position.z = mesh->mVertices[i].z;
            }

            if (mesh->HasNormals())
            {
                vertex.normal.x = mesh->mNormals[i].x;
                vertex.normal.y = mesh->mNormals[i].y;
                vertex.normal.z = mesh->mNormals[i].z;
            }

            if (mesh->HasTangentsAndBitangents())
            {
                vertex.tangent.x = mesh->mTangents[i].x;
                vertex.tangent.y = mesh->mTangents[i].y;
                vertex.tangent.z = mesh->mTangents[i].z;

                vertex.bitangent.x = mesh->mBitangents[i].x;
                vertex.bitangent.y = mesh->mBitangents[i].y;
                vertex.bitangent.z = mesh->mBitangents[i].z;
            }

            if (mesh->HasTextureCoords(0))
            {
                // A vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vertex.texCoords.x = mesh->mTextureCoords[0][i].x;
                vertex.texCoords.y = mesh->mTextureCoords[0][i].y;
            }
            else
                vertex.texCoords = glm::vec2(0.0f, 0.0f);

            retMeh.vertices.push_back(vertex);
        }
        // Now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
        {
            aiFace face = mesh->mFaces[i];
            // Retrieve all indices of the face and store them in the indices vector
            for (uint32_t j = 0; j < face.mNumIndices; ++j)
            {
                retMeh.indices.push_back(face.mIndices[j]);
            }
        }

        // Process materials
        if (mesh->mMaterialIndex >= 0)
        {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            loadMaterialTextures(retMeh, material, aiTextureType_AMBIENT);
            loadMaterialTextures(retMeh, material, aiTextureType_DIFFUSE);
            loadMaterialTextures(retMeh, material, aiTextureType_SPECULAR);
            loadMaterialTextures(retMeh, material, aiTextureType_HEIGHT);
            loadMaterialTextures(retMeh, material, aiTextureType_OPACITY);

            std::vector<Mesh::Texture> add;

            std::pair<std::string, aiTextureType> map_from[] = {
                { "_diff", aiTextureType_DIFFUSE } };

            std::pair<std::string, aiTextureType> map_to[] = {
                { "_spec", aiTextureType_SPECULAR } };

            for (auto &from_type : map_from)
            {
                for (auto &tex : retMeh.textures)
                {
                    if (from_type.second != tex.type)
                        continue;

                    for (auto &to_type : map_to)
                    {
                        std::string path = tex.path.C_Str();
                        size_t loc = path.find(from_type.first);
                        if (loc == std::string::npos)
                            continue;

                        path.replace(loc, from_type.first.size(), to_type.first);
                        if (std::ifstream(path).good())
                            continue;

                        Mesh::Texture texture;
                        texture.id = -1;
                        texture.type = to_type.second;
                        texture.path = path;
                        TextureFromFile(texture);
                        add.push_back(texture);
                    }
                }
            }
            retMeh.textures.insert(retMeh.textures.end(), add.begin(), add.end());

            aiColor4D amb;
            aiColor4D dif;
            aiColor4D spec;
            float shininess;
            aiString name;

            if (material->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
            {
                retMeh.material.shininess = shininess;
            }
            if (material->Get(AI_MATKEY_COLOR_AMBIENT, amb) == aiReturn_SUCCESS)
            {
                retMeh.material.amb = glm::vec3(aiColor4DToVec4(amb));
            }
            if (material->Get(AI_MATKEY_COLOR_DIFFUSE, dif) == aiReturn_SUCCESS)
            {
                retMeh.material.dif = glm::vec3(aiColor4DToVec4(dif));
            }
            if (material->Get(AI_MATKEY_COLOR_SPECULAR, spec) == aiReturn_SUCCESS)
            {
                retMeh.material.spec = max(glm::vec3(0.05f), glm::vec3(aiColor4DToVec4(spec)));
            }
            if (material->Get(AI_MATKEY_NAME, name) == aiReturn_SUCCESS)
            {
                retMeh.material.name = name;
            }
        }

        return retMeh;
    }

    void loadMaterialTextures(Mesh &retMeh, aiMaterial* mat, aiTextureType type)
    {
        for (uint32_t i = 0; i < mat->GetTextureCount(type); ++i)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            Mesh::Texture texture;
            texture.id = -1;
            texture.type = type;
            texture.path = m_directory + "/" + str.C_Str();
            TextureFromFile(texture);
            retMeh.textures.push_back(texture);
        }
    }

    void TextureFromFile(Mesh::Texture &texture)
    {
        return;
        int width, height;
        unsigned char* image = SOIL_load_image(texture.path.C_Str(), &width, &height, 0, SOIL_LOAD_RGBA);
        SOIL_free_image_data(image);
        texture.id = 0;
    }
};