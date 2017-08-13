#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <SimpleMath.h>
#include <Geometry.h>

using namespace Microsoft::WRL;
using namespace DirectX::SimpleMath;

struct CommandHelper;

struct DX12Mesh : IMesh
{
    virtual void AddVertex(const IVertex& ivertex) override
    {
        Vertex vertex;

        vertex.position.x = ivertex.position.x;
        vertex.position.y = ivertex.position.y;
        vertex.position.z = ivertex.position.z;

        vertex.normal.x = ivertex.normal.x;
        vertex.normal.y = ivertex.normal.y;
        vertex.normal.z = ivertex.normal.z;

        vertex.tangent.x = ivertex.tangent.x;
        vertex.tangent.y = ivertex.tangent.y;
        vertex.tangent.z = ivertex.tangent.z;

        vertex.bitangent.x = ivertex.bitangent.x;
        vertex.bitangent.y = ivertex.bitangent.y;
        vertex.bitangent.z = ivertex.bitangent.z;

        vertex.texCoords.x = ivertex.texCoords.x;
        vertex.texCoords.y = ivertex.texCoords.y;

        vertices.push_back(vertex);
    }

    virtual void AddIndex(const IIndex& iindex) override
    {
        indices.push_back(iindex);
    }

    virtual void AddTexture(const ITexture& itexture) override
    {
        Texture texture;

        texture.type = itexture.type;
        texture.path = itexture.path;

        textures.push_back(texture);
    }

    virtual void SetMaterial(const IMaterial& imaterial) override
    {
        material.amb.x = imaterial.amb.x;
        material.amb.y = imaterial.amb.y;
        material.amb.z = imaterial.amb.z;

        material.dif.x = imaterial.dif.x;
        material.dif.y = imaterial.dif.y;
        material.dif.z = imaterial.dif.z;

        material.spec.x = imaterial.spec.x;
        material.spec.y = imaterial.spec.y;
        material.spec.z = imaterial.spec.z;

        material.shininess = imaterial.shininess;

        material.name = imaterial.name;
    }

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

    void SetupMesh(CommandHelper commandHelper);
};

struct CommandHelper
{
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12GraphicsCommandList> commandList;

    CommandHelper(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList);
    void PushBuffer(DX12Mesh::Buffer& buffer, void* buffer_data, size_t buffer_size);
};
