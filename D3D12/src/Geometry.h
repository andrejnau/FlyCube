#pragma once

#include <Windows.h>
#include <d3dx12.h>
#include <d3d12.h>

#include <vector>
#include <fstream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Util.h"
#include <SimpleMath.h>
#include <wrl.h>

using namespace Microsoft::WRL;
using namespace DirectX::SimpleMath;

struct CommandHelper;

struct Mesh
{
    struct Vertex
    {
        Vector3 position;
        Vector3 normal;
        Vector2 texCoords;
        Vector3 tangent;
        Vector3 bitangent;
    };

    struct Buffer
    {
        ComPtr<ID3D12Resource> defaultHeap;
        ComPtr<ID3D12Resource> uploadHeap;
    };

    struct Texture
    {
        aiTextureType type;
        std::string path;
        uint32_t offset;
        Buffer buffer;
    };

    struct Material
    {
        Vector3 amb = Vector3(0.0, 0.0, 0.0);
        Vector3 dif = Vector3(1.0, 1.0, 1.0);
        Vector3 spec = Vector3(1.0, 1.0, 1.0);
        float shininess = 32.0;
        aiString name;
    } material;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<Texture> textures;

    ComPtr<ID3D12DescriptorHeap> currentDescriptorTextureHeap;

    Buffer vertexHeap;
    Buffer IndexHeap;

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW indexBufferView;

    void setupMesh(CommandHelper commandHelper);
};

class Model
{
public:
    Model(const std::string & file);

    std::string m_path;
    std::string m_directory;
    std::vector<Mesh> meshes;

    struct BoundBox
    {
        BoundBox();
        float x_min, y_min, z_min;
        float x_max, y_max, z_max;
    } boundBox;

private:
    std::string splitFilename(const std::string& str);
    void loadModel();

    void processNode(aiNode* node, const aiScene* scene);

    Vector3 aiColor4DToVec3(const aiColor4D& x);

    Mesh processMesh(aiMesh* mesh, const aiScene* scene);

    void loadMaterialTextures(Mesh &retMeh, aiMaterial* mat, aiTextureType type);
};

struct CommandHelper
{
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;

    CommandHelper(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList);
    void pushBuffer(Mesh::Buffer &buffer, void *buffer_data, size_t buffer_size);
};