#include "Geometry.h"
#include <vector>
#include <tuple>
#include <fstream>
#include <set>

ModelParser::ModelParser(const std::string& file, IModel& meshes)
    : m_path(ASSETS_PATH + file)
    , m_directory(SplitFilename(m_path))
    , m_model(meshes)
{
    LoadModel();
}

std::string ModelParser::SplitFilename(const std::string& str)
{
    return str.substr(0, str.find_last_of("/"));
}

void ModelParser::LoadModel()
{
    Assimp::Importer import;
    const aiScene* scene = import.ReadFile(m_path, aiProcess_FlipUVs | aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace);
    assert(scene && scene->mFlags != AI_SCENE_FLAGS_INCOMPLETE && scene->mRootNode);
    ProcessNode(scene->mRootNode, scene);
}

void ModelParser::ProcessNode(aiNode* node, const aiScene* scene)
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

void ModelParser::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
    IMesh& cur_mesh = m_model.GetNextMesh();
    // Walk through each of the mesh's vertices
    for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
    {
        IMesh::IVertex vertex;

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
            vertex.normal = mesh->mNormals[i];
        }

        if (mesh->HasTangentsAndBitangents())
        {
            vertex.tangent = mesh->mTangents[i];
            vertex.bitangent = mesh->mBitangents[i];
        }

        // A vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
        // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
        if (mesh->HasTextureCoords(0))
        {
            vertex.texCoords = aiVector2D(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }
        else
        {
            vertex.texCoords = aiVector2D(0.0f, 0.0f);
        }

        cur_mesh.AddVertex(vertex);
    }
    // Now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
    for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
    {
        aiFace face = mesh->mFaces[i];
        // Retrieve all indices of the face and store them in the indices vector
        for (uint32_t j = 0; j < face.mNumIndices; ++j)
        {
            cur_mesh.AddIndex(face.mIndices[j]);
        }
    }

    // Process materials
    if (mesh->mMaterialIndex >= 0)
    {
        std::vector<IMesh::ITexture> textures;
        aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
        LoadMaterialTextures(mat, aiTextureType_AMBIENT, textures);
        LoadMaterialTextures(mat, aiTextureType_DIFFUSE, textures);
        LoadMaterialTextures(mat, aiTextureType_SPECULAR, textures);
        LoadMaterialTextures(mat, aiTextureType_HEIGHT, textures);
        LoadMaterialTextures(mat, aiTextureType_OPACITY, textures);

        FindSimilarTextures(textures);

        auto comparator = [&](const IMesh::ITexture& lhs, const IMesh::ITexture& rhs)
        {
            return std::tie(lhs.type, lhs.path) < std::tie(rhs.type, rhs.path);
        };

        std::set<IMesh::ITexture, decltype(comparator)> unique_textures(comparator);

        for (IMesh::ITexture& texture : textures)
        {
            unique_textures.insert(texture);
        }

        for (const IMesh::ITexture& texture : unique_textures)
        {
            cur_mesh.AddTexture(texture);
        }

        IMesh::IMaterial material;

        mat->Get(AI_MATKEY_SHININESS, material.shininess);
        mat->Get(AI_MATKEY_COLOR_AMBIENT, material.amb);
        mat->Get(AI_MATKEY_COLOR_DIFFUSE, material.dif);
        mat->Get(AI_MATKEY_COLOR_SPECULAR, material.spec);
        mat->Get(AI_MATKEY_NAME, material.name);

        cur_mesh.SetMaterial(material);
    }
}

void ModelParser::FindSimilarTextures(std::vector<IMesh::ITexture>& textures)
{
    static std::pair<std::string, aiTextureType> map_from[] = {
        { "_s", aiTextureType_SPECULAR },
        { "_color", aiTextureType_DIFFUSE }
    };

    static std::pair<std::string, aiTextureType> map_to[] = {
        { "_g", aiTextureType_SHININESS },
        { "_gloss", aiTextureType_SHININESS },
        { "_rough", aiTextureType_SHININESS },
        { "_nmap", aiTextureType_HEIGHT }
    };

    std::vector<IMesh::ITexture> added_textures;
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

                IMesh::ITexture texture;
                texture.type = to_type.second;
                texture.path = cur_path;
                added_textures.push_back(texture);
            }
        }
    }

    textures.insert(textures.end(), added_textures.begin(), added_textures.end());
}

void ModelParser::LoadMaterialTextures(aiMaterial* mat, aiTextureType type, std::vector<IMesh::ITexture>& textures)
{
    for (uint32_t i = 0; i < mat->GetTextureCount(type); ++i)
    {
        aiString texture_name;
        mat->GetTexture(type, i, &texture_name);
        std::string texture_path = m_directory + "/" + texture_name.C_Str();
        if(!std::ifstream(texture_path).good())
            continue;

        IMesh::ITexture texture;
        texture.type = type;
        texture.path = texture_path;
        textures.push_back(texture);
    }
}
