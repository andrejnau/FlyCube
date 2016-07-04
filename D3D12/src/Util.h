#pragma once

#include <wincodec.h>

GUID GetTargetPixelFormat(const GUID& sourceFormat);
DXGI_FORMAT GetDXGIFormatFromPixelFormat(const GUID& pixelFormat);
size_t BitsPerPixel(_In_ DXGI_FORMAT fmt);

struct CommandHelper
{
    ID3D12Device* m_device;
    ID3D12GraphicsCommandList* m_commandList;


    CommandHelper(  ID3D12Device* device,
                    ID3D12GraphicsCommandList* commandList)
        : m_device(device)
        , m_commandList(commandList)
    {}

    ID3D12Resource* pushBuffer(void *buffer_data, size_t buffer_size)
    {
        ID3D12Resource* buffer_resource; // a default buffer in GPU memory that we will load vertex data for our triangle into

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
            IID_PPV_ARGS(&buffer_resource));

        // create upload heap
        // upload heaps are used to upload data to the GPU. CPU can write to it, GPU can read from it
        // We will upload the vertex buffer using this heap to the default heap
        ID3D12Resource* upload_heap;
        m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // upload heap
            D3D12_HEAP_FLAG_NONE, // no flags
            &CD3DX12_RESOURCE_DESC::Buffer(buffer_size), // resource description for a buffer
            D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
            nullptr,
            IID_PPV_ARGS(&upload_heap));

        // store vertex buffer in upload heap
        D3D12_SUBRESOURCE_DATA vertexData = {};
        vertexData.pData = reinterpret_cast<BYTE*>(buffer_data); // pointer to our vertex array
        vertexData.RowPitch = buffer_size; // size of all our triangle vertex data
        vertexData.SlicePitch = buffer_size; // also the size of our triangle vertex data

                                             // we are now creating a command with the command list to copy the data from
                                             // the upload heap to the default heap
        UpdateSubresources(m_commandList, buffer_resource, upload_heap, 0, 0, 1, &vertexData);

        // transition the vertex buffer data from copy destination state to vertex buffer state
        m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(buffer_resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
        return buffer_resource;
    }
};