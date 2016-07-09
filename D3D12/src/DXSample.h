#pragma once

#include "IDXSample.h"
#include "Win32Application.h"

#include <memory>
#include <string>

#include <d3dx12.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <SimpleMath.h>
#include <DXGI1_4.h>
#include <wrl.h>

#include "Geometry.h"

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::SimpleMath;

class DXSample : public IDXSample
{
public:
    DXSample(UINT width, UINT height);
    virtual ~DXSample();

    virtual void OnInit() override;
    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnDestroy() override;
    virtual void OnSizeChanged(int width, int height) override;

    UINT GetWidth() const  override;
    UINT GetHeight() const  override;

private:

    void CreateDevice();
    void CreateCommandQueue();
    void CreateSwapChain();
    void CreateFence();
    void CreateRT();
    void CreateDepthStencil();
    void CreateCommandAllocators();
    void CreateCommandList();
    void CreateGeometry();
    void CreateConstantBuffer();
    void CreateRootSignature();
    void CreatePSO();
    void CreateViewPort();
    void CreateTexture(Mesh::Texture &texture);
    void UploadAllResources();
    void WaitForGpu();
    void WaitForPreviousFrame();
    void PopulateCommandList();

private:

    const bool use_rotare = true;

    std::unique_ptr<Model> m_modelOfFile;

    UINT m_width;

    UINT m_height;

    static const int frameBufferCount = 3; // number of buffers we want, 2 for double buffering, 3 for tripple buffering

    ComPtr<ID3D12Device> device; // direct3d device

    ComPtr<IDXGISwapChain3> swapChain; // swapchain used to switch between render targets

    ComPtr<ID3D12CommandQueue> commandQueue; // container for command lists

    ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap; // a descriptor heap to hold resources like the render targets

    ComPtr<ID3D12Resource> renderTargets[frameBufferCount]; // number of render targets equal to buffer count

    ComPtr<ID3D12CommandAllocator> commandAllocator[frameBufferCount]; // we want enough allocators for each buffer * number of threads (we only have one thread)

    ComPtr<ID3D12GraphicsCommandList> commandList; // a command list we can record commands into, then execute them to render the frame

    ComPtr<ID3D12Fence> fence[frameBufferCount]; // an object that is locked while our command list is being executed by the gpu. We need as many
                                          //as we have allocators (more if we want to know when the gpu is finished with an asset)

    HANDLE fenceEvent; // a handle to an event when our fence is unlocked by the gpu

    UINT64 fenceValue[frameBufferCount]; // this value is incremented each frame. each fence will have its own value

    int frameIndex; // current rtv we are on

    int rtvDescriptorSize; // size of the rtv descriptor on the device (all front and back buffers will be the same size)

    ComPtr<IDXGIFactory4> dxgiFactory;

    ComPtr<ID3D12PipelineState> pipelineStateObject; // pso containing a pipeline state
    ComPtr<ID3D12PipelineState> pipelineStateObjectWithBlend; // pso containing a pipeline state

    ComPtr<ID3D12RootSignature> rootSignature; // root signature defines data shaders will access

    D3D12_VIEWPORT viewport; // area that output from rasterizer will be stretched to.

    D3D12_RECT scissorRect; // the area to draw in. pixels outside that area will not be drawn onto

    ComPtr<ID3D12Resource> depthStencilBuffer; // This is the memory for our depth buffer. it will also be used for a stencil buffer in a later tutorial

    ComPtr<ID3D12DescriptorHeap> dsDescriptorHeap; // This is a heap for our depth/stencil buffer descriptor

    // this is the structure of our constant buffer.
    struct ConstantBufferPerObject
    {
        Matrix model;
        Matrix view;
        Matrix projection;
        Vector4 lightPos;
        Vector4 viewPos;
    };

    // Constant buffers must be 256-byte aligned which has to do with constant reads on the GPU.
    // We are only able to read at 256 byte intervals from the start of a resource heap, so we will
    // make sure that we add padding between the two constant buffers in the heap (one for cube1 and one for cube2)
    // Another way to do this would be to add a float array in the constant buffer structure for padding. In this case
    // we would need to add a float padding[50]; after the wvpMat variable. This would align our structure to 256 bytes (4 bytes per float)
    // The reason i didn't go with this way, was because there would actually be wasted cpu cycles when memcpy our constant
    // buffer data to the gpu virtual address. currently we memcpy the size of our structure, which is 16 bytes here, but if we
    // were to add the padding array, we would memcpy 64 bytes if we memcpy the size of our structure, which is 50 wasted bytes
    // being copied.
    int ConstantBufferPerObjectAlignedSize = (sizeof(ConstantBufferPerObject) + 255) & ~255;

    ConstantBufferPerObject cbPerObject; // this is the constant buffer data we will send to the gpu
                                         // (which will be placed in the resource we created above)

    ComPtr<ID3D12Resource> constantBufferUploadHeaps[frameBufferCount]; // this is the memory on the gpu where constant buffers for each frame will be placed

    Matrix cameraProjMat; // this will store our projection matrix
    Matrix cameraViewMat; // this will store our view matrix
    Matrix cubeWorldMat; // our first cubes world matrix (transformation matrix)

    Vector4 cameraPosition; // this is our cameras position vector
    Vector4 cameraTarget; // a vector describing the point in space our camera is looking at
    Vector4 cameraUp; // the worlds up vector

    struct TexInfo
    {
        D3D12_RESOURCE_DESC resourceDescription;
        std::unique_ptr<uint8_t[]> imageData;
        int textureWidth;
        int textureHeight;
        int numBitsPerPixel;
        int imageSize;
        int bytesPerRow;
    };

    TexInfo DXSample::LoadImageDataFromFile(std::string filename);

    const static uint32_t max_texture_slot = 10;
    ComPtr<ID3D12DescriptorHeap> mainDescriptorTextureHeap;
};