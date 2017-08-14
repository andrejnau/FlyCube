#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <Geometry.h>
#include <glm/glm.hpp>

using namespace Microsoft::WRL;

struct CommandHelper;

struct DX12Mesh : IMesh
{
    struct Buffer
    {
        ComPtr<ID3D12Resource> defaultHeap;
        ComPtr<ID3D12Resource> uploadHeap;
    };

    struct TexResources
    {
        uint32_t offset;
        Buffer buffer;
    };

    std::vector<TexResources> texResources;

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
