#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <SimpleMath.h>
#include "Geometry.h"

using namespace Microsoft::WRL;
using namespace DirectX::SimpleMath;

struct DX11Mesh : IMesh
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

    struct Texture
    {
        aiTextureType type;
        std::string path;

        ComPtr<ID3D11ShaderResourceView> textureRV;
    };

    struct Material
    {
        Vector3 amb = Vector3(0.0, 0.0, 0.0);
        Vector3 dif = Vector3(1.0, 1.0, 1.0);
        Vector3 spec = Vector3(1.0, 1.0, 1.0);
        float shininess = 32.0;
        aiString name;
    } material;

    ComPtr<ID3D11Buffer> vertBuffer;
    ComPtr<ID3D11Buffer> indexBuffer;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<Texture> textures;

    void SetupMesh(ComPtr<ID3D11Device>& d3d11Device, ComPtr<ID3D11DeviceContext>& d3d11DevCon)
    {
        D3D11_BUFFER_DESC vertexBufferDesc;
        ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

        vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        vertexBufferDesc.ByteWidth = vertices.size() * sizeof(vertices[0]);
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vertexBufferDesc.CPUAccessFlags = 0;
        vertexBufferDesc.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA vertexBufferData;

        ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
        vertexBufferData.pSysMem = vertices.data();
        HRESULT hr = d3d11Device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &vertBuffer);

        D3D11_BUFFER_DESC indexBufferDesc;
        ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));
        indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        indexBufferDesc.ByteWidth = indices.size() * sizeof(indices[0]);
        indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        indexBufferDesc.CPUAccessFlags = 0;
        indexBufferDesc.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA iinitData;
        iinitData.pSysMem = indices.data();
        hr = d3d11Device->CreateBuffer(&indexBufferDesc, &iinitData, &indexBuffer);
    }
};
