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
        size_t cnt = 0;
        for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                continue;
            }

            if (++cnt < 1)
                continue;

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

    ASSERT_SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
    ASSERT_SUCCEEDED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

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
    frame_index = swap_chain->GetCurrentBackBufferIndex();

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

#if defined(_DEBUG)
     ComPtr<ID3D12InfoQueue> d3dInfoQueue;
     if (SUCCEEDED(device.As(&d3dInfoQueue)))
     {
         d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
         d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
     }
#endif

     descriptor_pool.reset(new DescriptorPool(*this));
}

Resource::Ptr DX12Context::GetBackBuffer()
{
    DX12Resource::Ptr res = std::make_shared<DX12Resource>();
    ComPtr<ID3D12Resource> back_buffer;
    ASSERT_SUCCEEDED(swap_chain->GetBuffer(frame_index, IID_PPV_ARGS(&back_buffer)));
    res->state = D3D12_RESOURCE_STATE_PRESENT;
    res->default_res = back_buffer;
    return res;
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

std::vector<DX12ProgramApi*> tmp;

void DX12Context::Present(const Resource::Ptr& ires)
{
    ResourceBarrier(std::static_pointer_cast<DX12Resource>(ires), D3D12_RESOURCE_STATE_PRESENT);

    commandList->Close();
    ID3D12CommandList* ppCommandLists[] = { commandList.Get() };

    // execute the array of command lists
    commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    ASSERT_SUCCEEDED(swap_chain->Present(1, 0));

    WaitForPreviousFrame();

    ASSERT_SUCCEEDED(commandAllocator->Reset());

    ASSERT_SUCCEEDED(commandList->Reset(commandAllocator.Get(), nullptr));

    descriptor_pool->OnFrameBegin();
    for (auto & x : tmp)
        x->OnPresent();
}

void DX12Context::DrawIndexed(UINT IndexCount)
{
    current_program->ApplyBindings();
    commandList->DrawIndexedInstanced(IndexCount, 1, 0, 0, 0);
}

void DX12Context::Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
{
    current_program->ApplyBindings();
    commandList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
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
    scissorRect.right = width;
    scissorRect.bottom = height;
    commandList->RSSetScissorRects(1, &scissorRect); // set the scissor rects
}

void DX12Context::OMSetRenderTargets(std::vector<Resource::Ptr> rtv, Resource::Ptr dsv)
{
    if (!current_program)
        return;

    current_program->OMSetRenderTargets(rtv, dsv);
}

void DX12Context::ClearRenderTarget(Resource::Ptr rtv, const FLOAT ColorRGBA[4])
{
}

void DX12Context::ClearDepthStencil(Resource::Ptr dsv, UINT ClearFlags, FLOAT Depth, UINT8 Stencil)
{
}

Resource::Ptr DX12Context::CreateTexture(uint32_t bind_flag, DXGI_FORMAT format, uint32_t msaa_count, int width, int height, int depth, int mip_levels)
{
    DX12Resource::Ptr res = std::make_shared<DX12Resource>();

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = depth;
    desc.MipLevels = mip_levels;
    desc.Format = format;

    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS ms_check_desc = {};
    ms_check_desc.Format = desc.Format;
    ms_check_desc.SampleCount = msaa_count;
    device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &ms_check_desc, sizeof(ms_check_desc));
    desc.SampleDesc.Count = msaa_count;
    desc.SampleDesc.Quality = ms_check_desc.NumQualityLevels - 1;

    if (bind_flag & BindFlag::kRtv)
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    if (bind_flag & BindFlag::kDsv)
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    if (bind_flag & BindFlag::kUav)
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    res->state = D3D12_RESOURCE_STATE_COPY_DEST;

    ASSERT_SUCCEEDED(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &desc,
        res->state,
        nullptr,
        IID_PPV_ARGS(&res->default_res)));

    return res;
}

Resource::Ptr DX12Context::CreateBuffer(uint32_t bind_flag, UINT buffer_size, size_t stride)
{
    if (buffer_size == 0)
        return DX12Resource::Ptr();

    DX12Resource::Ptr res = std::make_shared<DX12Resource>();

    if (bind_flag & BindFlag::kCbv)
        buffer_size = (buffer_size + 255) & ~255;

    auto desc = CD3DX12_RESOURCE_DESC::Buffer(buffer_size);

    if (bind_flag & BindFlag::kRtv)
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    if (bind_flag & BindFlag::kDsv)
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    if (bind_flag & BindFlag::kUav)
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    res->stride = stride;
    res->state = D3D12_RESOURCE_STATE_COPY_DEST;

    device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &desc,
        res->state,
        nullptr,
        IID_PPV_ARGS(&res->default_res));

    return res;
}

void DX12Context::UpdateSubresource(const Resource::Ptr& ires, UINT DstSubresource, const void * pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch, size_t version)
{
    auto res = std::static_pointer_cast<DX12Resource>(ires);

    auto& upload_res = res->GetUploadResource(DstSubresource + version);
    if (!upload_res)
    {
        UINT64 buffer_size = GetRequiredIntermediateSize(res->default_res.Get(), DstSubresource, 1);
        device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(buffer_size),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&upload_res));
    }

    D3D12_SUBRESOURCE_DATA data = {};
    data.pData = pSrcData;
    data.RowPitch = SrcRowPitch;
    data.SlicePitch = SrcDepthPitch;

    ResourceBarrier(res, D3D12_RESOURCE_STATE_COPY_DEST);

    UpdateSubresources(commandList.Get(), res->default_res.Get(), upload_res.Get(), 0, DstSubresource, 1, &data);
}


std::unique_ptr<ProgramApi> DX12Context::CreateProgram()
{
    auto res =  std::make_unique<DX12ProgramApi>(*this);
    tmp.push_back(res.get());
    return res;
}

void DX12Context::IASetIndexBuffer(Resource::Ptr ires, UINT SizeInBytes, DXGI_FORMAT Format)
{
    auto res = std::static_pointer_cast<DX12Resource>(ires);
    D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
    indexBufferView.Format = Format;
    indexBufferView.SizeInBytes = SizeInBytes;
    indexBufferView.BufferLocation = res->default_res->GetGPUVirtualAddress();
    commandList->IASetIndexBuffer(&indexBufferView);
    ResourceBarrier(res, D3D12_RESOURCE_STATE_INDEX_BUFFER);
}

void DX12Context::IASetVertexBuffer(UINT slot, Resource::Ptr ires, UINT SizeInBytes, UINT Stride)
{
    auto res = std::static_pointer_cast<DX12Resource>(ires);
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
    vertexBufferView.BufferLocation = res->default_res->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = SizeInBytes;
    vertexBufferView.StrideInBytes = Stride;
    commandList->IASetVertexBuffers(slot, 1, &vertexBufferView);
    ResourceBarrier(res, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
}

#include <pix_win.h>

void DX12Context::BeginEvent(LPCWSTR Name)
{
    PIXBeginEvent(commandList.Get(), 0, Name);
}

void DX12Context::EndEvent()
{
    PIXEndEvent(commandList.Get());
}

void DX12Context::ResizeBackBuffer(int width, int height)
{
}
