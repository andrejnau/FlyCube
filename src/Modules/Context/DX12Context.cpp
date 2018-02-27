#include "Context/DX12Context.h"
#include <Utilities/DXUtility.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>


#include <Program/DX12ProgramApi.h>

DX12Context::DX12Context(GLFWwindow* window, int width, int height)
    : Context(window, width, height)
{
#if defined(_DEBUG)
    // Enable the D3D12 debug layer.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
        }
    }
#endif

    ComPtr<IDXGIFactory4> dxgi_factory;
    ASSERT_SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory)));

    auto GetHardwareAdapter = [&](ComPtr<IDXGIFactory4> dxgiFactory) -> ComPtr<IDXGIAdapter1>
    {
        ComPtr<IDXGIAdapter1> adapter; // adapters are the graphics card (this includes the embedded graphics on the motherboard)
        for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                continue;
            }

            // Check to see if the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_1, _uuidof(ID3D12Device), nullptr)))
                return adapter;
        }
        return nullptr;
    };

    ComPtr<IDXGIAdapter1> adapter = GetHardwareAdapter(dxgi_factory.Get());
    ASSERT(adapter != nullptr);
    ASSERT_SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&device)));

    D3D12_COMMAND_QUEUE_DESC cqDesc = {}; // we will be using all the default values
    ASSERT_SUCCEEDED(device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&commandQueue))); // create the command queue

    ASSERT_SUCCEEDED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgi_factory)));

    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
    swap_chain_desc.Width = width;
    swap_chain_desc.Height = height;
    swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.BufferCount = frameBufferCount;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    ComPtr<IDXGISwapChain1> tmp_swap_chain;
    ASSERT_SUCCEEDED(dxgi_factory->CreateSwapChainForHwnd(commandQueue.Get(), glfwGetWin32Window(window), &swap_chain_desc, nullptr, nullptr, &tmp_swap_chain));
    tmp_swap_chain.As(&swap_chain);

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        ASSERT_SUCCEEDED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fenceValue = 1;

        // Create an event handle to use for frame synchronization.
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr)
        {
            ASSERT_SUCCEEDED(HRESULT_FROM_WIN32(GetLastError()));
        }

        // Wait for the command list to execute; we are reusing the same command 
        // list in our main loop but for now, we just want to wait for setup to 
        // complete before continuing.
        WaitForPreviousFrame();
    }

     ASSERT_SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
     ASSERT_SUCCEEDED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

}

ComPtr<IUnknown> DX12Context::GetBackBuffer()
{
    ComPtr<ID3D12Resource> back_buffer;
    ASSERT_SUCCEEDED(swap_chain->GetBuffer(frame_index, IID_PPV_ARGS(&back_buffer)));
    return back_buffer;
}

void DX12Context::WaitForPreviousFrame()
{
    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
    // sample illustrates how to use fences for efficient resource usage and to
    // maximize GPU utilization.

    // Signal and increment the fence value.
    const UINT64 fence = m_fenceValue;
    ASSERT_SUCCEEDED(commandQueue->Signal(m_fence.Get(), fence));
    m_fenceValue++;

    // Wait until the previous frame is finished.
    if (m_fence->GetCompletedValue() < fence)
    {
        ASSERT_SUCCEEDED(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    frame_index = swap_chain->GetCurrentBackBufferIndex();
}


void DX12Context::Present()
{
    commandList->Close();
    ID3D12CommandList* ppCommandLists[] = { commandList.Get() };

    // execute the array of command lists
    commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    ASSERT_SUCCEEDED(swap_chain->Present(1, 0));

    WaitForPreviousFrame();

    ASSERT_SUCCEEDED(commandAllocator->Reset());

    ASSERT_SUCCEEDED(commandList->Reset(commandAllocator.Get(), nullptr));
}

void DX12Context::DrawIndexed(UINT IndexCount)
{
    commandList->DrawIndexedInstanced(IndexCount, 1, 0, 0, 0);
}

void DX12Context::Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
{
}

void DX12Context::SetViewport(int width, int height)
{
    D3D12_VIEWPORT viewport; // area that output from rasterizer will be stretched to.
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = (FLOAT)width;
    viewport.Height = (FLOAT)height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    commandList->RSSetViewports(1, &viewport); // set the viewports

    D3D12_RECT scissorRect; // the area to draw in. pixels outside that area will not be drawn onto
    scissorRect.left = 0;
    scissorRect.top = 0;
    scissorRect.right = m_width;
    scissorRect.bottom = m_height;
    commandList->RSSetScissorRects(1, &scissorRect); // set the scissor rects
}

void DX12Context::OMSetRenderTargets(std::vector<ComPtr<IUnknown>> rtv, ComPtr<IUnknown> dsv)
{
    if (!current_program)
        return;

    int slot = 0;
    for (auto& x : rtv)
    {
        current_program->AttachRTV(slot++, x);
    }

    current_program->AttachDSV(dsv);

    current_program->UpdateOmSet();
}

void DX12Context::ClearRenderTarget(ComPtr<IUnknown> rtv, const FLOAT ColorRGBA[4])
{
}

void DX12Context::ClearDepthStencil(ComPtr<IUnknown> dsv, UINT ClearFlags, FLOAT Depth, UINT8 Stencil)
{
}

ComPtr<IUnknown> DX12Context::CreateTexture(uint32_t bind_flag, DXGI_FORMAT format, uint32_t msaa_count, int width, int height, int depth, int mip_levels)
{
    D3D12_RESOURCE_DESC resourceDescription;
    // now describe the texture with the information we have obtained from the image
    resourceDescription = {};
    resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDescription.Alignment = 0; // may be 0, 4KB, 64KB, or 4MB. 0 will let runtime decide between 64KB and 4MB (4MB for multi-sampled textures)
    resourceDescription.Width = width; // width of the texture
    resourceDescription.Height = height; // height of the texture
    resourceDescription.DepthOrArraySize = depth; // if 3d image, depth of 3d image. Otherwise an array of 1D or 2D textures (we only have one image, so we set 1)
    resourceDescription.MipLevels = mip_levels; // Number of mipmaps. We are not generating mipmaps for this texture, so we have only one level
    resourceDescription.Format = format; // This is the dxgi format of the image (format of the pixels)
    resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // The arrangement of the pixels. Setting to unknown lets the driver choose the most efficient one
    resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE; // no flags


    if (bind_flag & BindFlag::kRtv)
        resourceDescription.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    if (bind_flag & BindFlag::kDsv)
        resourceDescription.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    if (bind_flag & BindFlag::kUav)
        resourceDescription.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS ms_desc = {};
    ms_desc.Format = format;
    ms_desc.SampleCount = msaa_count;
    device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &ms_desc, sizeof(ms_desc));
    resourceDescription.SampleDesc.Count = msaa_count;
    resourceDescription.SampleDesc.Quality = ms_desc.NumQualityLevels - 1;

    ComPtr<ID3D12Resource> defaultHeap;

    // create a default heap where the upload heap will copy its contents into (contents being the texture)
    ASSERT_SUCCEEDED(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
        D3D12_HEAP_FLAG_NONE, // no flags
        &resourceDescription, // the description of our texture
        D3D12_RESOURCE_STATE_COPY_DEST, // We will copy the texture from the upload heap to here, so we start it out in a copy dest state
        nullptr, // used for render targets and depth/stencil buffers
        IID_PPV_ARGS(&defaultHeap)));

    return defaultHeap;
}

ComPtr<IUnknown> DX12Context::CreateSamplerAnisotropic()
{
    return ComPtr<IUnknown>();
}

ComPtr<IUnknown> DX12Context::CreateSamplerShadow()
{
    return ComPtr<IUnknown>();
}

ComPtr<IUnknown> DX12Context::CreateShadowRSState()
{
    return ComPtr<IUnknown>();
}

void DX12Context::RSSetState(ComPtr<IUnknown> state)
{
}

std::unique_ptr<ProgramApi> DX12Context::CreateProgram()
{
    return std::make_unique<DX12ProgramApi>(*this);
}

ComPtr<IUnknown> DX12Context::CreateBuffer(uint32_t bind_flag, UINT buffer_size, size_t stride, const std::string & name)
{
    ComPtr<ID3D12Resource> defaultHeap;
    if (buffer_size == 0)
        return defaultHeap;

    if (bind_flag & BindFlag::kCbv)
        buffer_size = (buffer_size + 255) & ~255;
    // create default heap
    // default heap is memory on the GPU. Only the GPU has access to this memory
    // To get data into this heap, we will have to upload the data using
    // an upload heap
    device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
        D3D12_HEAP_FLAG_NONE, // no flags
        &CD3DX12_RESOURCE_DESC::Buffer(buffer_size), // resource description for a buffer
        D3D12_RESOURCE_STATE_COPY_DEST, // we will start this heap in the copy destination state since we will copy data
                                        // from the upload heap to this heap
        nullptr, // optimized clear value must be null for this type of resource. used for render targets and depth/stencil buffers
        IID_PPV_ARGS(&defaultHeap));

    defaultHeap->SetPrivateData(buffer_stride, sizeof(stride), &stride);
    defaultHeap->SetPrivateData(buffer_bind_flag, sizeof(bind_flag), &bind_flag);
    defaultHeap->SetName(utf8_to_wstring(name).c_str());
    return defaultHeap;
}

void DX12Context::IASetIndexBuffer(ComPtr<IUnknown> res, UINT SizeInBytes, DXGI_FORMAT Format)
{
    ComPtr<ID3D12Resource> buf;
    res.As(&buf);
    D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
    indexBufferView.Format = Format;
    indexBufferView.SizeInBytes = SizeInBytes;
    indexBufferView.BufferLocation = buf->GetGPUVirtualAddress();
    commandList->IASetIndexBuffer(&indexBufferView);
}

void DX12Context::IASetVertexBuffer(UINT slot, ComPtr<IUnknown> res, UINT SizeInBytes, UINT Stride)
{
    ComPtr<ID3D12Resource> buf;
    res.As(&buf);
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
    vertexBufferView.BufferLocation = buf->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = SizeInBytes;
    vertexBufferView.StrideInBytes = Stride;
    commandList->IASetVertexBuffers(slot, 1, &vertexBufferView);
}

void DX12Context::UpdateSubresource(ComPtr<IUnknown> ires, UINT DstSubresource, const void * pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch)
{
    ComPtr<ID3D12Resource> defaultHeap;
    ires.As(&defaultHeap);

    D3D12_RESOURCE_DESC desc = defaultHeap->GetDesc();

    UINT64 textureUploadBufferSize;
    // this function gets the size an upload buffer needs to be to upload a texture to the gpu.
    // each row must be 256 byte aligned except for the last row, which can just be the size in bytes of the row
    // eg. textureUploadBufferSize = ((((width * numBytesPerPixel) + 255) & ~255) * (height - 1)) + (width * numBytesPerPixel);
    //textureUploadBufferSize = (((imageBytesPerRow + 255) & ~255) * (textureDesc.Height - 1)) + imageBytesPerRow;
    device->GetCopyableFootprints(&desc, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

    ComPtr<ID3D12Resource> uploadHeap;
    UINT buffer_size = textureUploadBufferSize;
    // create upload heap
    // upload heaps are used to upload data to the GPU. CPU can write to it, GPU can read from it
    // We will upload the vertex buffer using this heap to the default heap
    ComPtr<ID3D12Resource> upload_heap;
    device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // upload heap
        D3D12_HEAP_FLAG_NONE, // no flags
        &CD3DX12_RESOURCE_DESC::Buffer(buffer_size), // resource description for a buffer
        D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
        nullptr,
        IID_PPV_ARGS(&uploadHeap));

    static std::vector<ComPtr<IUnknown>> w;
    w.push_back(uploadHeap);
    w.push_back(ires);

    // store vertex buffer in upload heap
    D3D12_SUBRESOURCE_DATA vertexData = {};
    vertexData.pData = pSrcData; // pointer to our vertex array
    vertexData.RowPitch = SrcRowPitch; // size of all our triangle vertex data
    vertexData.SlicePitch = SrcDepthPitch; // also the size of our triangle vertex data

    UINT bind_flag = 0;
    UINT stride_size = sizeof(bind_flag);
    auto hr = defaultHeap->GetPrivateData(buffer_bind_flag, &stride_size, &bind_flag);
    D3D12_RESOURCE_STATES next_state = D3D12_RESOURCE_STATE_COMMON;
    if (bind_flag & BindFlag::kVbv)
        next_state = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    if (bind_flag & BindFlag::kCbv)
        next_state = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    if (bind_flag & BindFlag::kIbv)
        next_state = D3D12_RESOURCE_STATE_INDEX_BUFFER;
    if (bind_flag & BindFlag::kSrv)
        next_state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    
   // if (next_state == D3D12_RESOURCE_STATE_COMMON)
    //    return;

    // we are now creating a command with the command list to copy the data from
    // the upload heap to the default heap
    UpdateSubresources(commandList.Get(), defaultHeap.Get(), uploadHeap.Get(), 0, 0, 1, &vertexData);





    // transition the vertex buffer data from copy destination state to vertex buffer state
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultHeap.Get(), D3D12_RESOURCE_STATE_COPY_DEST, next_state));
}

void DX12Context::BeginEvent(LPCWSTR Name)
{
}

void DX12Context::EndEvent()
{
}

void DX12Context::ResizeBackBuffer(int width, int height)
{
}
