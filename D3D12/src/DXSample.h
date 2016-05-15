#pragma once

#include "IDXSample.h"

#include <d3d12.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DXGI1_4.h>
#include <Windows.h>

#include <d3dx12.h>
#include <DirectXMath.h>

#include "Win32Application.h"

class DXSample : public IDXSample
{
public:
    DXSample(UINT width, UINT height);
    virtual ~DXSample();

    virtual void OnInit() override;
    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnDestroy() override;

    UINT GetWidth() const  override;
    UINT GetHeight() const  override;

private:

    UINT m_width;
    UINT m_height;
    float m_aspectRatio;

    // direct3d stuff
    static const int frameBufferCount = 3; // number of buffers we want, 2 for double buffering, 3 for tripple buffering

    ID3D12Device* device; // direct3d device

    IDXGISwapChain3* swapChain; // swapchain used to switch between render targets

    ID3D12CommandQueue* commandQueue; // container for command lists

    ID3D12DescriptorHeap* rtvDescriptorHeap; // a descriptor heap to hold resources like the render targets

    ID3D12Resource* renderTargets[frameBufferCount]; // number of render targets equal to buffer count

    ID3D12CommandAllocator* commandAllocator[frameBufferCount]; // we want enough allocators for each buffer * number of threads (we only have one thread)

    ID3D12GraphicsCommandList* commandList; // a command list we can record commands into, then execute them to render the frame

    ID3D12Fence* fence[frameBufferCount];    // an object that is locked while our command list is being executed by the gpu. We need as many 
                                             //as we have allocators (more if we want to know when the gpu is finished with an asset)

    HANDLE fenceEvent; // a handle to an event when our fence is unlocked by the gpu

    UINT64 fenceValue[frameBufferCount]; // this value is incremented each frame. each fence will have its own value

    int frameIndex; // current rtv we are on

    int rtvDescriptorSize; // size of the rtv descriptor on the device (all front and back buffers will be the same size)

                           //void Update(); // update the game logic


    IDXGIFactory4* dxgiFactory;


    ID3D12PipelineState* pipelineStateObject; // pso containing a pipeline state

    ID3D12RootSignature* rootSignature; // root signature defines data shaders will access

    D3D12_VIEWPORT viewport; // area that output from rasterizer will be stretched to.

    D3D12_RECT scissorRect; // the area to draw in. pixels outside that area will not be drawn onto

    ID3D12Resource* vertexBuffer; // a default buffer in GPU memory that we will load vertex data for our triangle into

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView; // a structure containing a pointer to the vertex data in gpu memory
                                               // the total size of the buffer, and the size of each element (vertex)

    struct Vertex
    {
        Vertex(float x, float y, float z, float r, float g, float b, float a) :
            pos(x, y, z),
            color(r, g, b, z)
        {}

        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT4 color;
    };

    ID3D12Resource* indexBuffer; // a default buffer in GPU memory that we will load index data for our triangle into

    D3D12_INDEX_BUFFER_VIEW indexBufferView; // a structure holding information about the index buffer

    ID3D12Resource* depthStencilBuffer; // This is the memory for our depth buffer. it will also be used for a stencil buffer in a later tutorial
    ID3D12DescriptorHeap* dsDescriptorHeap; // This is a heap for our depth/stencil buffer descriptor

    // this is the structure of our constant buffer.
    struct ConstantBuffer
    {
        DirectX::XMFLOAT4 colorMultiplier;
    };

    ID3D12DescriptorHeap* mainDescriptorHeap[frameBufferCount]; // this heap will store the descripor to our constant buffer
    ID3D12Resource* constantBufferUploadHeap[frameBufferCount]; // this is the memory on the gpu where our constant buffer will be placed.

    ConstantBuffer cbColorMultiplierData; // this is the constant buffer data we will send to the gpu
                                          // (which will be placed in the resource we created above)

    UINT8* cbColorMultiplierGPUAddress[frameBufferCount]; // this is a pointer to the memory location we get when we map our constant buffer

    void createDivice();
    void createSwapChain();
    void createCommandQueqe();
    void createRT();
    void createCommandAllocators();
    void createCommandList();
    void createFence();
    void createRootSignature();
    void createPSO();
    void createVertexAndIndexBuffer();
    void initViewPort();
    void WaitForPreviousFrame();
    void UpdatePipeline();
};