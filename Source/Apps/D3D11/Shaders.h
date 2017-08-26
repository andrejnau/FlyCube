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

#include <IDXSample.h>
#include <Geometry.h>
#include <Util.h>
#include <Utility.h>
#include <FileUtility.h>

#include "DX11Geometry.h"

using namespace Microsoft::WRL;
using namespace DirectX;

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
    };

    UniformBuffer uniform;
    ComPtr<ID3D11Buffer> uniform_buffer;

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
            { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DX11Mesh::Vertex, bitangent), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        UINT numElements = ARRAYSIZE(layout);

        //Create the Input Layout
        ASSERT_SUCCEEDED(m_device->CreateInputLayout(layout, numElements, vertex_shader_buffer->GetBufferPointer(),
            vertex_shader_buffer->GetBufferSize(), &input_layout));

        D3D11_BUFFER_DESC cbbd;
        ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));
        cbbd.Usage = D3D11_USAGE_DEFAULT;
        cbbd.ByteWidth = sizeof(uniform);
        cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbbd.CPUAccessFlags = 0;
        cbbd.MiscFlags = 0;
        ASSERT_SUCCEEDED(m_device->CreateBuffer(&cbbd, nullptr, &uniform_buffer));
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

    UniformBuffer uniform;
    ComPtr<ID3D11Buffer> uniform_buffer;

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
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DX11Mesh::Vertex, normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(DX11Mesh::Vertex, texCoords), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DX11Mesh::Vertex, tangent), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DX11Mesh::Vertex, bitangent), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        UINT numElements = ARRAYSIZE(layout);

        //Create the Input Layout
        ASSERT_SUCCEEDED(m_device->CreateInputLayout(layout, numElements, vertex_shader_buffer->GetBufferPointer(),
            vertex_shader_buffer->GetBufferSize(), &input_layout));

        D3D11_BUFFER_DESC cbbd;
        ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));
        cbbd.Usage = D3D11_USAGE_DEFAULT;
        cbbd.ByteWidth = sizeof(uniform);
        cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbbd.CPUAccessFlags = 0;
        cbbd.MiscFlags = 0;
        ASSERT_SUCCEEDED(m_device->CreateBuffer(&cbbd, nullptr, &uniform_buffer));
    }
};
