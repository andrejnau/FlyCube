#include "Geometry.h"

void Mesh::setupMesh(CommandHelper commandHelper)
{
    commandHelper.pushBuffer(vertexHeap, vertices.data(), vertices.size() * sizeof(vertices[0]));
    vertexBufferView.BufferLocation = vertexHeap.defaultHeap->GetGPUVirtualAddress();
    vertexBufferView.StrideInBytes = sizeof(vertices[0]);
    vertexBufferView.SizeInBytes = vertices.size() * sizeof(vertices[0]);

    commandHelper.pushBuffer(IndexHeap, indices.data(), indices.size() * sizeof(indices[0]));
    indexBufferView.BufferLocation = IndexHeap.defaultHeap->GetGPUVirtualAddress();
    indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    indexBufferView.SizeInBytes = indices.size() * sizeof(indices[0]);
}

Model::Model(const std::string & file)
    : m_path(PROJECT_RESOURCE_DIR + file)
    , m_directory(splitFilename(m_path))
{
    loadModel();
}

inline std::string Model::splitFilename(const std::string & str)
{
    return str.substr(0, str.find_last_of("/"));
}

inline void Model::loadModel()
{
    Assimp::Importer import;
    const aiScene* scene = import.ReadFile(m_path, aiProcess_FlipUVs | aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace);
    assert(scene && scene->mFlags != AI_SCENE_FLAGS_INCOMPLETE && scene->mRootNode);
    processNode(scene->mRootNode, scene);
}

inline void Model::processNode(aiNode * node, const aiScene * scene)
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

inline Vector3 Model::aiColor4DToVec3(const aiColor4D & x)
{
    return Vector3(x.r, x.g, x.b);
}

inline Mesh Model::processMesh(aiMesh * mesh, const aiScene * scene)
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

            boundBox.x_min = std::min(boundBox.x_min, vertex.position.x);
            boundBox.x_max = std::max(boundBox.x_max, vertex.position.x);

            boundBox.y_min = std::min(boundBox.y_min, vertex.position.y);
            boundBox.y_max = std::max(boundBox.y_max, vertex.position.y);

            boundBox.z_min = std::min(boundBox.z_min, vertex.position.z);
            boundBox.z_max = std::max(boundBox.z_max, vertex.position.z);
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
            vertex.texCoords = Vector2(0.0f, 0.0f);

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
            { "_s", aiTextureType_SPECULAR },
            { "_color", aiTextureType_DIFFUSE }
        };

        std::pair<std::string, aiTextureType> map_to[] = {
            { "_g", aiTextureType_SHININESS },
            { "_gloss", aiTextureType_SHININESS },
            { "_rough", aiTextureType_SHININESS },
            { "_nmap", aiTextureType_HEIGHT }
        };

        for (auto &from_type : map_from)
        {
            for (auto &tex : retMeh.textures)
            {
                if (from_type.second != tex.type)
                    continue;

                for (auto &to_type : map_to)
                {
                    std::string path = tex.path;
                    size_t loc = path.find(from_type.first);
                    if (loc == std::string::npos)
                        continue;

                    path.replace(loc, from_type.first.size(), to_type.first);
                    if (!std::ifstream(path).good())
                        continue;

                    Mesh::Texture texture;
                    texture.type = to_type.second;
                    texture.path = path;
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
            retMeh.material.amb = Vector3(aiColor4DToVec3(amb));
        }
        if (material->Get(AI_MATKEY_COLOR_DIFFUSE, dif) == aiReturn_SUCCESS)
        {
            retMeh.material.dif = Vector3(aiColor4DToVec3(dif));
        }
        if (material->Get(AI_MATKEY_COLOR_SPECULAR, spec) == aiReturn_SUCCESS)
        {
            retMeh.material.spec = Vector3(aiColor4DToVec3(spec));
        }
        if (material->Get(AI_MATKEY_NAME, name) == aiReturn_SUCCESS)
        {
            retMeh.material.name = name;
        }
    }

    return retMeh;
}

inline void Model::loadMaterialTextures(Mesh & retMeh, aiMaterial * mat, aiTextureType type)
{
    for (uint32_t i = 0; i < mat->GetTextureCount(type); ++i)
    {
        aiString texture_name;
        mat->GetTexture(type, i, &texture_name);
        std::string texture_path = m_directory + "/" + texture_name.C_Str();
        if(!std::ifstream(texture_path).good())
            continue;

        Mesh::Texture texture;
        texture.type = type;
        texture.path = texture_path;
        retMeh.textures.push_back(texture);
    }
}

inline Model::BoundBox::BoundBox()
{
    x_min = y_min = z_min = std::numeric_limits<float>::max();
    x_max = y_max = z_max = std::numeric_limits<float>::min();
}

CommandHelper::CommandHelper(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList)
    : m_device(device)
    , m_commandList(commandList)
{}

inline void CommandHelper::pushBuffer(Mesh::Buffer & buffer, void * buffer_data, size_t buffer_size)
{
    // create default heap
    // default heap is memory on the GPU. Only the GPU has access to this memory
    // To get data into this heap, we will have to upload the data using
    // an upload heap
    m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
        D3D12_HEAP_FLAG_NONE, // no flags
        &CD3DX12_RESOURCE_DESC::Buffer(buffer_size), // resource description for a buffer
        D3D12_RESOURCE_STATE_COPY_DEST, // we will start this heap in the copy destination state since we will copy data
                                        // from the upload heap to this heap
        nullptr, // optimized clear value must be null for this type of resource. used for render targets and depth/stencil buffers
        IID_PPV_ARGS(&buffer.defaultHeap));

    // create upload heap
    // upload heaps are used to upload data to the GPU. CPU can write to it, GPU can read from it
    // We will upload the vertex buffer using this heap to the default heap
    ComPtr<ID3D12Resource> upload_heap;
    m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // upload heap
        D3D12_HEAP_FLAG_NONE, // no flags
        &CD3DX12_RESOURCE_DESC::Buffer(buffer_size), // resource description for a buffer
        D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
        nullptr,
        IID_PPV_ARGS(&buffer.uploadHeap));

    // store vertex buffer in upload heap
    D3D12_SUBRESOURCE_DATA vertexData = {};
    vertexData.pData = reinterpret_cast<BYTE*>(buffer_data); // pointer to our vertex array
    vertexData.RowPitch = buffer_size; // size of all our triangle vertex data
    vertexData.SlicePitch = buffer_size; // also the size of our triangle vertex data

                                         // we are now creating a command with the command list to copy the data from
                                         // the upload heap to the default heap
    UpdateSubresources(m_commandList.Get(), buffer.defaultHeap.Get(), buffer.uploadHeap.Get(), 0, 0, 1, &vertexData);

    // transition the vertex buffer data from copy destination state to vertex buffer state
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(buffer.defaultHeap.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
}
