#include "Context/DX12Context.h"

#include <pix_win.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <Utilities/DXUtility.h>
#include <Utilities/State.h>
#include <Program/DX12ProgramApi.h>

// Note that Windows 10 Creator Update SDK is required for enabling Shader Model 6 feature.
static HRESULT EnableExperimentalShaderModels()
{
    static const GUID D3D12ExperimentalShaderModelsID = { /* 76f5573e-f13a-40f5-b297-81ce9e18933f */
        0x76f5573e,
        0xf13a,
        0x40f5,
    { 0xb2, 0x97, 0x81, 0xce, 0x9e, 0x18, 0x93, 0x3f }
    };

    return D3D12EnableExperimentalFeatures(1, &D3D12ExperimentalShaderModelsID, nullptr, nullptr);
}

DX12Context::DX12Context(GLFWwindow* window, int width, int height)
    : Context(window, width, height)
{
    auto& state = CurState<bool>::Instance().state;
    if (state["DXIL"])
        EnableExperimentalShaderModels();

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
    ASSERT_SUCCEEDED(device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&m_command_queue))); // create the command queue

    ASSERT_SUCCEEDED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgi_factory)));

    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
    swap_chain_desc.Width = width;
    swap_chain_desc.Height = height;
    swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.BufferCount = FrameCount;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    ComPtr<IDXGISwapChain1> tmp_swap_chain;
    ASSERT_SUCCEEDED(dxgi_factory->CreateSwapChainForHwnd(m_command_queue.Get(), glfwGetWin32Window(window), &swap_chain_desc, nullptr, nullptr, &tmp_swap_chain));
    tmp_swap_chain.As(&m_swap_chain);
    m_frame_index = m_swap_chain->GetCurrentBackBufferIndex();

    for (size_t i = 0; i < FrameCount; ++i)
    {
        ASSERT_SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_command_allocator[i])));
    }

    ASSERT_SUCCEEDED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_command_allocator[m_frame_index].Get(), nullptr, IID_PPV_ARGS(&command_list)));

    {
        ASSERT_SUCCEEDED(device->CreateFence(m_fence_values[m_frame_index], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fence_values[m_frame_index]++;
        m_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
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

std::unique_ptr<ProgramApi> DX12Context::CreateProgram()
{
    auto res = std::make_unique<DX12ProgramApi>(*this);
    m_created_program.push_back(*res.get());
    return res;
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

    D3D12_CLEAR_VALUE* p_clear_value = nullptr;
    D3D12_CLEAR_VALUE clear_value = {};
    clear_value.Format = format;
    if (bind_flag & BindFlag::kRtv)
    {
        clear_value.Color[0] = 0.0f;
        clear_value.Color[1] = 0.2f;
        clear_value.Color[2] = 0.4f;
        clear_value.Color[3] = 1.0f;
        p_clear_value = &clear_value;
    }
    else if (bind_flag & BindFlag::kDsv)
    {
        clear_value.DepthStencil.Depth = 1.0f;
        clear_value.DepthStencil.Stencil = 0.0f;
        if (format == DXGI_FORMAT_R32_TYPELESS)
            clear_value.Format = DXGI_FORMAT_D32_FLOAT;
        p_clear_value = &clear_value;
    }

    ASSERT_SUCCEEDED(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &desc,
        res->state,
        p_clear_value,
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

void DX12Context::UpdateSubresource(const Resource::Ptr& ires, UINT DstSubresource, const void * pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch)
{
    auto res = std::static_pointer_cast<DX12Resource>(ires);

    auto& upload_res = res->GetUploadResource(DstSubresource);
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

    UpdateSubresources(command_list.Get(), res->default_res.Get(), upload_res.Get(), 0, DstSubresource, 1, &data);
}

void DX12Context::SetViewport(int width, int height)
{
    D3D12_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = (FLOAT)width;
    viewport.Height = (FLOAT)height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    command_list->RSSetViewports(1, &viewport);

    SetScissorRect(0, 0, width, height);
}

void DX12Context::SetScissorRect(LONG left, LONG top, LONG right, LONG bottom)
{
    D3D12_RECT rect = { left, top, right, bottom };
    command_list->RSSetScissorRects(1, &rect);
}

void DX12Context::IASetIndexBuffer(Resource::Ptr ires, UINT SizeInBytes, DXGI_FORMAT Format)
{
    auto res = std::static_pointer_cast<DX12Resource>(ires);
    D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
    indexBufferView.Format = Format;
    indexBufferView.SizeInBytes = SizeInBytes;
    indexBufferView.BufferLocation = res->default_res->GetGPUVirtualAddress();
    command_list->IASetIndexBuffer(&indexBufferView);
    ResourceBarrier(res, D3D12_RESOURCE_STATE_INDEX_BUFFER);
}

void DX12Context::IASetVertexBuffer(UINT slot, Resource::Ptr ires, UINT SizeInBytes, UINT Stride)
{
    auto res = std::static_pointer_cast<DX12Resource>(ires);
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
    vertexBufferView.BufferLocation = res->default_res->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = SizeInBytes;
    vertexBufferView.StrideInBytes = Stride;
    command_list->IASetVertexBuffers(slot, 1, &vertexBufferView);
    ResourceBarrier(res, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
}

void DX12Context::BeginEvent(LPCWSTR Name)
{
    PIXBeginEvent(command_list.Get(), 0, Name);
}

void DX12Context::EndEvent()
{
    PIXEndEvent(command_list.Get());
}

void DX12Context::DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
    current_program->ApplyBindings();
    command_list->DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
}

void DX12Context::Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
{
    current_program->ApplyBindings();
    command_list->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

Resource::Ptr DX12Context::GetBackBuffer()
{
    DX12Resource::Ptr res = std::make_shared<DX12Resource>();
    ComPtr<ID3D12Resource> back_buffer;
    ASSERT_SUCCEEDED(m_swap_chain->GetBuffer(m_frame_index, IID_PPV_ARGS(&back_buffer)));
    res->state = D3D12_RESOURCE_STATE_PRESENT;
    res->default_res = back_buffer;
    return res;
}

void DX12Context::Present(const Resource::Ptr& ires)
{
    ResourceBarrier(std::static_pointer_cast<DX12Resource>(ires), D3D12_RESOURCE_STATE_PRESENT);

    command_list->Close();
    ID3D12CommandList* ppCommandLists[] = { command_list.Get() };

    m_command_queue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    ASSERT_SUCCEEDED(m_swap_chain->Present(0, 0));

    MoveToNextFrame();

    ASSERT_SUCCEEDED(m_command_allocator[m_frame_index]->Reset());
    ASSERT_SUCCEEDED(command_list->Reset(m_command_allocator[m_frame_index].Get(), nullptr));

    if (m_frame_index == 0)
        descriptor_pool->OnFrameBegin();
    for (auto & x : m_created_program)
        x.get().OnPresent();
}

void DX12Context::ResourceBarrier(const DX12Resource::Ptr& res, D3D12_RESOURCE_STATES state)
{
    if (res->state != state)
        command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(res->default_res.Get(), res->state, state));
    res->state = state;
}

void DX12Context::UseProgram(DX12ProgramApi& program_api)
{
    current_program = &program_api;
}

void DX12Context::ResizeBackBuffer(int width, int height)
{
}

void DX12Context::MoveToNextFrame()
{
    // Schedule a Signal command in the queue.
    const UINT64 currentFenceValue = m_fence_values[m_frame_index];
    ASSERT_SUCCEEDED(m_command_queue->Signal(m_fence.Get(), currentFenceValue));

    // Update the frame index.
    m_frame_index = m_swap_chain->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is ready.
    if (m_fence->GetCompletedValue() < m_fence_values[m_frame_index])
    {
        ASSERT_SUCCEEDED(m_fence->SetEventOnCompletion(m_fence_values[m_frame_index], m_fence_event));
        WaitForSingleObjectEx(m_fence_event, INFINITE, FALSE);
    }

    // Set the fence value for the next frame.
    m_fence_values[m_frame_index] = currentFenceValue + 1;
}
