#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <glm/glm.hpp>
#include <Geometry/Geometry.h>

using namespace Microsoft::WRL;

struct DX11Mesh : IMesh
{
    ComPtr<ID3D11Buffer> vertBuffer;
    ComPtr<ID3D11Buffer> indexBuffer;

    std::vector<ComPtr<ID3D11ShaderResourceView>> texResources;

    void SetupMesh(ComPtr<ID3D11Device>& d3d11Device, ComPtr<ID3D11DeviceContext>& d3d11DevCon)
    {
        texResources.resize(textures.size());
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
