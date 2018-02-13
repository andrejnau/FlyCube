#include "Geometry/Geometry.h"
#include <Utilities/FileUtility.h>
#include <vector>
#include <tuple>
#include <fstream>
#include <set>
#include <map>

glm::vec3 aiVector3DToVec3(const aiVector3D& x)
{
    return glm::vec3(x.x, x.y, x.z);
}

ModelLoader::ModelLoader(const std::string& file, aiPostProcessSteps flags, IModel& meshes)
    : m_path(GetAssetFullPath(file))
    , m_directory(SplitFilename(m_path))
    , m_model(meshes)
    , m_bones(meshes.GetBones())
{
    LoadModel(flags);
}

std::string ModelLoader::SplitFilename(const std::string& str)
{
    return str.substr(0, str.find_last_of("/"));
}

void ModelLoader::LoadModel(aiPostProcessSteps flags)
{
    // aiProcess_PreTransformVertices
    const aiScene* scene = import.ReadFile(m_path, flags & (aiProcess_FlipUVs | aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes |  aiProcess_CalcTangentSpace));
    assert(scene && scene->mFlags != AI_SCENE_FLAGS_INCOMPLETE && scene->mRootNode);
    ProcessNode(scene->mRootNode, scene);
}

void ModelLoader::ProcessNode(aiNode* node, const aiScene* scene)
{
    for (uint32_t i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessMesh(mesh, scene);
    }

    for (uint32_t i = 0; i < node->mNumChildren; ++i)
    {
        ProcessNode(node->mChildren[i], scene);
    }
}

inline glm::vec4 aiColor4DToVec4(const aiColor4D& x)
{
    return glm::vec4(x.r, x.g, x.b, x.a);
}

bool SkipMesh(aiMesh* mesh, const aiScene* scene)
{
    if (mesh->mMaterialIndex >= scene->mNumMaterials)
        return false;
    aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
    aiString name;
    if (!mat->Get(AI_MATKEY_NAME, name) == AI_SUCCESS)
        return false;
    std::set<std::string> q = {
        /*"hair_outer",
        "hair_inner",*/
        "fur",
        "16___Default"
    };
    if (mesh->mNumBones == 0)
    {
        volatile int b = 0;
    }
    return q.count(std::string(name.C_Str()));
}

void ModelLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
    if (SkipMesh(mesh, scene))
        return;

    IMesh cur_mesh = {};
    // Walk through each of the mesh's vertices
    for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
    {
        struct Vertex
        {
            glm::vec3 position;
            glm::vec3 normal;
            glm::vec2 texcoord;
            glm::vec3 tangent;
            glm::vec3 bitangent;
        } vertex;

        if (mesh->HasPositions())
        {
            vertex.position.x = mesh->mVertices[i].x;
            vertex.position.y = mesh->mVertices[i].y;
            vertex.position.z = mesh->mVertices[i].z;

            m_model.GetBoundBox().x_min = std::min(m_model.GetBoundBox().x_min, vertex.position.x);
            m_model.GetBoundBox().x_max = std::max(m_model.GetBoundBox().x_max, vertex.position.x);

            m_model.GetBoundBox().y_min = std::min(m_model.GetBoundBox().y_min, vertex.position.y);
            m_model.GetBoundBox().y_max = std::max(m_model.GetBoundBox().y_max, vertex.position.y);

            m_model.GetBoundBox().z_min = std::min(m_model.GetBoundBox().z_min, vertex.position.z);
            m_model.GetBoundBox().z_max = std::max(m_model.GetBoundBox().z_max, vertex.position.z);
        }

        if (mesh->HasNormals())
        {
            vertex.normal = aiVector3DToVec3(mesh->mNormals[i]);
        }

        if (mesh->HasTangentsAndBitangents())
        {
            vertex.tangent = aiVector3DToVec3(mesh->mTangents[i]);
            vertex.bitangent = aiVector3DToVec3(mesh->mBitangents[i]);

            if (glm::dot(glm::cross(vertex.normal, vertex.tangent), vertex.bitangent) < 0.0f)
            {
                vertex.tangent *= -1.0f;
            }
        }

        // A vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
        // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
        if (mesh->HasTextureCoords(0))
        {
            vertex.texcoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }
        else
        {
            vertex.texcoord = glm::vec2(0.0f, 0.0f);
        }

        cur_mesh.positions.push_back(vertex.position);
        cur_mesh.normals.push_back(vertex.normal);
        cur_mesh.texcoords.push_back(vertex.texcoord);
        cur_mesh.tangents.push_back(vertex.tangent);
    }
    // Now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
    for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
    {
        aiFace face = mesh->mFaces[i];
        // Retrieve all indices of the face and store them in the indices vector
        for (uint32_t j = 0; j < face.mNumIndices; ++j)
        {
            cur_mesh.indices.push_back(face.mIndices[j]);
        }
    }

    // Process materials
    if (mesh->mMaterialIndex >= 0)
    {
        std::vector<TextureInfo> textures;
        aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
        LoadMaterialTextures(mat, aiTextureType_AMBIENT, textures);
        LoadMaterialTextures(mat, aiTextureType_DIFFUSE, textures);
        LoadMaterialTextures(mat, aiTextureType_SPECULAR, textures);
        LoadMaterialTextures(mat, aiTextureType_HEIGHT, textures);
        LoadMaterialTextures(mat, aiTextureType_OPACITY, textures);

        FindSimilarTextures(textures);

        auto comparator = [&](const TextureInfo& lhs, const TextureInfo& rhs)
        {
            return std::tie(lhs.type, lhs.path) < std::tie(rhs.type, rhs.path);
        };

        std::set<TextureInfo, decltype(comparator)> unique_textures(comparator);

        for (TextureInfo& texture : textures)
        {
            unique_textures.insert(texture);
        }

        for (const TextureInfo& texture : unique_textures)
        {
            cur_mesh.textures.push_back(texture);
        }

        aiColor4D amb;
        if (mat->Get(AI_MATKEY_COLOR_AMBIENT, amb) == AI_SUCCESS)
            cur_mesh.material.amb = glm::vec3(aiColor4DToVec4(amb));

        aiColor4D dif;
        if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, dif) == AI_SUCCESS)
            cur_mesh.material.dif = glm::vec3(aiColor4DToVec4(dif));

        aiColor4D spec;
        if (mat->Get(AI_MATKEY_COLOR_SPECULAR, spec) == AI_SUCCESS)
            cur_mesh.material.spec = glm::vec3(aiColor4DToVec4(spec));

        float shininess;
        if (mat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
            cur_mesh.material.shininess = shininess;

        aiString name;
        if (mat->Get(AI_MATKEY_NAME, name) == AI_SUCCESS)
            cur_mesh.material.name = name.C_Str();
    }

    m_bones.LoadBones(mesh, scene, cur_mesh);

    m_model.AddMesh(cur_mesh);
}

void ModelLoader::FindSimilarTextures(std::vector<TextureInfo>& textures)
{
    static std::pair<std::string, aiTextureType> map_from[] = {
        { "_s", aiTextureType_SPECULAR },
        { "_diff", aiTextureType_DIFFUSE },
        { "_color", aiTextureType_DIFFUSE }
    };

    static std::pair<std::string, aiTextureType> map_to[] = {
        { "_g", aiTextureType_SHININESS },
        { "_gloss", aiTextureType_SHININESS },
        { "_rough", aiTextureType_SHININESS },
        { "_nmap", aiTextureType_HEIGHT },
        { "_spec", aiTextureType_SPECULAR },
    };

    std::vector<TextureInfo> added_textures;
    for (auto& from_type : map_from)
    {
        for (auto& cur_texture : textures)
        {
            std::string path = cur_texture.path;

            size_t loc = path.find(from_type.first);
            if (loc == std::string::npos)
                continue;

            for (auto& to_type : map_to)
            {
                std::string cur_path = path;
                cur_path.replace(loc, from_type.first.size(), to_type.first);
                if (!std::ifstream(cur_path).good())
                    continue;

                TextureInfo texture;
                texture.type = to_type.second;
                texture.path = cur_path;
                added_textures.push_back(texture);
            }
        }
    }

    textures.insert(textures.end(), added_textures.begin(), added_textures.end());
}

void ModelLoader::LoadMaterialTextures(aiMaterial* mat, aiTextureType type, std::vector<TextureInfo>& textures)
{
    for (uint32_t i = 0; i < mat->GetTextureCount(type); ++i)
    {
        aiString texture_name;
        mat->GetTexture(type, i, &texture_name);
        std::string texture_path = m_directory + "/" + texture_name.C_Str();
        if(!std::ifstream(texture_path).good())
            continue;

        TextureInfo texture;
        texture.type = type;
        texture.path = texture_path;
        textures.push_back(texture);
    }
}
