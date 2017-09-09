#pragma once

#include <d3d11.h>

#include <d3d11.h>
#include <DXGI1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <memory>
#include <string>

#include <Geometry/Geometry.h>
#include <Util.h>
#include <Utility.h>
#include <Utilities/FileUtility.h>

#include "DX11Geometry.h"

using namespace Microsoft::WRL;
using namespace DirectX;

template<typename T>
class CBuffer
{
public:
    void CreateBuffer(ComPtr<ID3D11Device>& device)
    {
        D3D11_BUFFER_DESC cbbd;
        ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));
        cbbd.Usage = D3D11_USAGE_DEFAULT;
        cbbd.ByteWidth = UpTo(sizeof(T), 16);
        cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbbd.CPUAccessFlags = 0;
        cbbd.MiscFlags = 0;
        ASSERT_SUCCEEDED(device->CreateBuffer(&cbbd, nullptr, &buffer));
    }
    
    void Update(ComPtr<ID3D11DeviceContext>& device_context)
    {
        device_context->UpdateSubresource(buffer.Get(), 0, nullptr, &data, 0, 0);
    }

    T data;
    ComPtr<ID3D11Buffer> buffer;

private:
    size_t UpTo(size_t n, size_t m)
    {
        return n + (m - n % m) % m;
    }
};

struct ShaderGeometryPass
{
    ComPtr<ID3D11VertexShader> vertex_shader;
    ComPtr<ID3D11PixelShader> pixel_shader;
    ComPtr<ID3DBlob> vertex_shader_buffer;
    ComPtr<ID3DBlob> pixel_shader_buffer;
    ComPtr<ID3D11InputLayout> input_layout;

    struct UniformBuffer
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
        glm::mat4 normalMatrix;
    };

    CBuffer<UniformBuffer> uniform;

    struct TexturesEnables
    {
        int has_diffuseMap;
        int has_normalMap;
        int has_specularMap;
        int has_glossMap;
        int has_ambientMap;
        int has_alphaMap;
    };

    CBuffer<TexturesEnables> textures_enables;

    struct Material
    {
        glm::vec3 ambient; uint8_t _0[4];
        glm::vec3 diffuse; uint8_t _1[4];
        glm::vec3 specular;
        float shininess;
    };

    CBuffer<Material> material;

    struct Light
    {
        glm::vec3 ambient; uint8_t _0[4];
        glm::vec3 diffuse; uint8_t _1[4];
        glm::vec3 specular; uint8_t _2[4];
    };

    CBuffer<Light> light;

    ShaderGeometryPass(ComPtr<ID3D11Device> m_device)
    {
        ComPtr<ID3DBlob> pErrors;
        ASSERT_SUCCEEDED(D3DCompileFromFile(
            GetAssetFullPath(L"shaders/DX11/GeometryPass_VS.hlsl").c_str(),
            nullptr,
            nullptr,
            "main",
            "vs_5_0",
            0,
            0,
            &vertex_shader_buffer,
            &pErrors));

        ASSERT_SUCCEEDED(D3DCompileFromFile(
            GetAssetFullPath(L"shaders/DX11/GeometryPass_PS.hlsl").c_str(),
            nullptr,
            nullptr,
            "main",
            "ps_5_0",
            0,
            0,
            &pixel_shader_buffer,
            &pErrors));

        //Create the Shader Objects
        ASSERT_SUCCEEDED(m_device->CreateVertexShader(vertex_shader_buffer->GetBufferPointer(), vertex_shader_buffer->GetBufferSize(), nullptr, &vertex_shader));
        ASSERT_SUCCEEDED(m_device->CreatePixelShader(pixel_shader_buffer->GetBufferPointer(), pixel_shader_buffer->GetBufferSize(), nullptr, &pixel_shader));

        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DX11Mesh::Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DX11Mesh::Vertex, normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(DX11Mesh::Vertex, texCoords), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DX11Mesh::Vertex, tangent), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        UINT numElements = ARRAYSIZE(layout);

        //Create the Input Layout
        ASSERT_SUCCEEDED(m_device->CreateInputLayout(layout, numElements, vertex_shader_buffer->GetBufferPointer(),
            vertex_shader_buffer->GetBufferSize(), &input_layout));

        uniform.CreateBuffer(m_device);
        textures_enables.CreateBuffer(m_device);
        material.CreateBuffer(m_device);
        light.CreateBuffer(m_device);
    }
};

struct ShaderLightPass
{
    ComPtr<ID3D11VertexShader> vertex_shader;
    ComPtr<ID3D11PixelShader> pixel_shader;
    ComPtr<ID3DBlob> vertex_shader_buffer;
    ComPtr<ID3DBlob> pixel_shader_buffer;
    ComPtr<ID3D11InputLayout> input_layout;

    struct UniformBuffer
    {
        glm::vec4 lightPos;
        glm::vec4 viewPos;
    };

    CBuffer<UniformBuffer> uniform;

    ShaderLightPass(ComPtr<ID3D11Device> m_device)
    {
        ComPtr<ID3DBlob> pErrors;
        ASSERT_SUCCEEDED(D3DCompileFromFile(
            GetAssetFullPath(L"shaders/DX11/LightPass_VS.hlsl").c_str(),
            nullptr,
            nullptr,
            "main",
            "vs_5_0",
            0,
            0,
            &vertex_shader_buffer,
            &pErrors));

        ASSERT_SUCCEEDED(D3DCompileFromFile(
            GetAssetFullPath(L"shaders/DX11/LightPass_PS.hlsl").c_str(),
            nullptr,
            nullptr,
            "main",
            "ps_5_0",
            0,
            0,
            &pixel_shader_buffer,
            &pErrors));

        //Create the Shader Objects
        ASSERT_SUCCEEDED(m_device->CreateVertexShader(vertex_shader_buffer->GetBufferPointer(), vertex_shader_buffer->GetBufferSize(), nullptr, &vertex_shader));
        ASSERT_SUCCEEDED(m_device->CreatePixelShader(pixel_shader_buffer->GetBufferPointer(), pixel_shader_buffer->GetBufferSize(), nullptr, &pixel_shader));

        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DX11Mesh::Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(DX11Mesh::Vertex, texCoords), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        UINT numElements = ARRAYSIZE(layout);

        //Create the Input Layout
        ASSERT_SUCCEEDED(m_device->CreateInputLayout(layout, numElements, vertex_shader_buffer->GetBufferPointer(),
            vertex_shader_buffer->GetBufferSize(), &input_layout));

        uniform.CreateBuffer(m_device);
    }
};
