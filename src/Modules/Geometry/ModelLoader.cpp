#include "Geometry/ModelLoader.h"
#include "Geometry/Model.h"
#include <Utilities/FileUtility.h>
#include <vector>
#include <set>

#include <assimp/DefaultIOStream.h>
#include <assimp/DefaultIOSystem.h>
#include <assimp/MemoryIOWrapper.h>

class MyIOSystem : public Assimp::DefaultIOSystem
{
public:
    Assimp::IOStream* Open(const char* strFile, const char* strMode) override
    {
        FILE* file = fopen(strFile, strMode);
        if (file == nullptr)
            return nullptr;

        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        fseek(file, 0, SEEK_SET);

        std::unique_ptr<uint8_t[]> buf(new uint8_t[size]);
        fread(buf.get(), sizeof(uint8_t), size, file);
        fclose(file);

        return new Assimp::MemoryIOStream(buf.release(), size, true);
    }
};

glm::vec3 aiVector3DToVec3(const aiVector3D& x)
{
    return glm::vec3(x.x, x.y, x.z);
}

ModelLoader::ModelLoader(const std::string& path, aiPostProcessSteps flags, IModel& model)
    : m_path(path)
    , m_directory(SplitFilename(m_path))
    , m_model(model)
{
    m_import.SetIOHandler(new MyIOSystem());
    LoadModel(flags);
}

std::string ModelLoader::SplitFilename(const std::string& str)
{
    return str.substr(0, str.find_last_of("/"));
}

void ModelLoader::LoadModel(aiPostProcessSteps flags)
{
    const aiScene* scene = m_import.ReadFile(m_path, flags & (aiProcess_FlipUVs | aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes |  aiProcess_CalcTangentSpace));
    assert(scene && scene->mFlags != AI_SCENE_FLAGS_INCOMPLETE && scene->mRootNode);
    m_model.GetBones().LoadModel(scene);
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
    static std::set<std::string> q = {
        "16___Default",
        "Ground_SG"
    };
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
        aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
        aiString name;
        if (mat->Get(AI_MATKEY_NAME, name) == AI_SUCCESS)
            cur_mesh.material.name = name.C_Str();

        std::vector<TextureInfo> textures;
        // map_Kd
        LoadMaterialTextures(mat, aiTextureType_DIFFUSE, TextureType::kAlbedo, textures);
        // map_bump
        LoadMaterialTextures(mat, aiTextureType_NORMALS, TextureType::kNormal, textures);
        // map_Ns
        LoadMaterialTextures(mat, aiTextureType_SHININESS, TextureType::kRoughness, textures);
        // map_Ks
        LoadMaterialTextures(mat, aiTextureType_SPECULAR, TextureType::kMetalness, textures);
        // map_d
        LoadMaterialTextures(mat, aiTextureType_OPACITY, TextureType::kOpacity, textures);

        FindSimilarTextures(cur_mesh.material.name, textures);

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
    }

    m_model.GetBones().ProcessMesh(mesh, cur_mesh);
    m_model.AddMesh(cur_mesh);
}

void ModelLoader::FindSimilarTextures(const std::string& mat_name, std::vector<TextureInfo>& textures)
{
    static std::pair<std::string, TextureType> texture_types[] = {
        { "albedo",     TextureType::kAlbedo     },
        { "_albedo",    TextureType::kAlbedo     },
        { "_Albedo",    TextureType::kAlbedo     },
        { "_color",     TextureType::kAlbedo     },
        { "_diff",      TextureType::kAlbedo     },
        { "_diffuse",   TextureType::kAlbedo     },
        { "_BaseColor", TextureType::kAlbedo     },
        { "_nmap",      TextureType::kNormal     },
        { "normal",     TextureType::kNormal     },
        { "_normal",    TextureType::kNormal     },
        { "_Normal",    TextureType::kNormal     },
        { "_rough",     TextureType::kRoughness  },
        { "roughness",  TextureType::kRoughness  },
        { "_roughness", TextureType::kRoughness  },
        { "_Roughness", TextureType::kRoughness  },
        { "_gloss",     TextureType::kGlossiness },
        { "_metalness", TextureType::kMetalness  },
        { "_metallic",  TextureType::kMetalness  },
        { "metallic",   TextureType::kMetalness  },
        { "_Metallic",  TextureType::kMetalness  },   
        { "_ao",        TextureType::kOcclusion  },
        { "ao",         TextureType::kOcclusion  },
        { "_mask",      TextureType::kOpacity    },
        { "_opacity",   TextureType::kOpacity    },
    };
    std::set<TextureType> used;
    for (auto& cur_texture : textures)
    {
        used.insert(cur_texture.type);
    }

    if (!used.count(TextureType::kAlbedo))
    {
        for (auto & ext : { ".dds", ".png", ".jpg" })
        {
            std::string cur_path = m_directory + "/textures/" + mat_name + "_albedo" + ext;
            if (std::ifstream(cur_path).good())
            {
                textures.push_back({ TextureType::kAlbedo, cur_path });
            }
            cur_path = m_directory + "/" + "albedo" + ext;
            if (std::ifstream(cur_path).good())
            {
                textures.push_back({ TextureType::kAlbedo, cur_path });
            }
        }
    }

    std::vector<TextureInfo> added_textures;
    for (auto& from_type : texture_types)
    {
        for (auto& cur_texture : textures)
        {
            std::string path = cur_texture.path;

            size_t loc = path.find(from_type.first);
            if (loc == std::string::npos)
                continue;

            for (auto& to_type : texture_types)
            {
                if (used.count(to_type.second))
                {
                    continue;
                }
                std::string cur_path = path;
                cur_path.replace(loc, from_type.first.size(), to_type.first);
                if (!std::ifstream(cur_path).good())
                    continue;

                TextureInfo texture;
                texture.type = to_type.second;
                texture.path = cur_path;
                added_textures.push_back(texture);
                used.insert(to_type.second);
            }
        }
    }

    textures.insert(textures.end(), added_textures.begin(), added_textures.end());
}

void ModelLoader::LoadMaterialTextures(aiMaterial* mat, aiTextureType aitype, TextureType type, std::vector<TextureInfo>& textures)
{
    for (uint32_t i = 0; i < mat->GetTextureCount(aitype); ++i)
    {
        aiString texture_name;
        mat->GetTexture(aitype, i, &texture_name);
        std::string texture_path = m_directory + "/" + texture_name.C_Str();
        if (!std::ifstream(texture_path).good())
        {
            texture_path = texture_path.substr(0, texture_path.rfind('.')) + ".dds";
            if (!std::ifstream(texture_path).good())
            {
                continue;
            }
        }

        TextureInfo texture;
        texture.type = type;
        texture.path = texture_path;
        textures.push_back(texture);
    }
}
