#include "Context/DX12Context.h"

#include <pix.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <Utilities/DXUtility.h>
#include <Utilities/State.h>
#include <Utilities/FileUtility.h>
#include <Program/DX12ProgramApi.h>
#include "Context/DXGIUtility.h"

DX12Context::DX12Context(GLFWwindow* window)
    : Context(window)
    , m_view_pool(*this)
{
#if 0
    static const GUID D3D12ExperimentalShaderModelsID = { /* 76f5573e-f13a-40f5-b297-81ce9e18933f */
        0x76f5573e,
        0xf13a,
        0x40f5,
    { 0xb2, 0x97, 0x81, 0xce, 0x9e, 0x18, 0x93, 0x3f }
    };
    ASSERT_SUCCEEDED(D3D12EnableExperimentalFeatures(1, &D3D12ExperimentalShaderModelsID, nullptr, nullptr));
#endif

#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debug_controller;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller))))
    {
        debug_controller->EnableDebugLayer();
    }
#endif

    ComPtr<IDXGIFactory4> dxgi_factory;
    ASSERT_SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory)));

    ComPtr<IDXGIAdapter1> adapter = GetHardwareAdapter(dxgi_factory.Get());
    ASSERT_SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&device)));
    device.As(&device5);

    D3D12_FEATURE_DATA_D3D12_OPTIONS5 feature_support = {};
    ASSERT_SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &feature_support, sizeof(feature_support)));
    m_use_render_passes = m_use_render_passes && feature_support.RenderPassesTier >= D3D12_RENDER_PASS_TIER_0;
    m_is_dxr_supported = feature_support.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;

    D3D12_COMMAND_QUEUE_DESC cq_desc = {};
    ASSERT_SUCCEEDED(device->CreateCommandQueue(&cq_desc, IID_PPV_ARGS(&m_command_queue)));

    m_swap_chain = CreateSwapChain(m_command_queue, dxgi_factory, glfwGetWin32Window(window), m_width, m_height, FrameCount);
    m_frame_index = m_swap_chain->GetCurrentBackBufferIndex();
    InitBackBuffers();

    for (size_t i = 0; i < FrameCount; ++i)
    {
        ASSERT_SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_command_allocator[i])));
    }

    ASSERT_SUCCEEDED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_command_allocator[m_frame_index].Get(), nullptr, IID_PPV_ARGS(&command_list)));
    command_list.As(&command_list4);

    ASSERT_SUCCEEDED(device->CreateFence(m_fence_values[m_frame_index], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    ++m_fence_values[m_frame_index];
    m_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    descriptor_pool.reset(new DescriptorPool(*this));

#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue> info_queue;
    if (SUCCEEDED(device.As(&info_queue)))
    {
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        // TODO: false positives for CopyDescriptors
        //info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
    }
#endif
}

DX12Context::~DX12Context()
{
    WaitForGpu();
    CloseHandle(m_fence_event);
}

std::unique_ptr<ProgramApi> DX12Context::CreateProgram()
{
    auto res = std::make_unique<DX12ProgramApi>(*this);
    m_created_program.push_back(*res.get());
    return res;
}

Resource::Ptr DX12Context::CreateTexture(uint32_t bind_flag, gli::format Format, uint32_t msaa_count, int width, int height, int depth, int mip_levels)
{
    DXGI_FORMAT format = static_cast<DXGI_FORMAT>(gli::dx().translate(Format).DXGIFormat.DDS);
    if (bind_flag & BindFlag::kSrv && format == DXGI_FORMAT_D32_FLOAT)
        format = DXGI_FORMAT_R32_TYPELESS;

    DX12Resource::Ptr res = std::make_shared<DX12Resource>(*this);

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
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    if (bind_flag & BindFlag::kDsv)
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    if (bind_flag & BindFlag::kUav)
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    res->state = D3D12_RESOURCE_STATE_COPY_DEST;

    D3D12_CLEAR_VALUE* p_clear_value = nullptr;
    D3D12_CLEAR_VALUE clear_value = {};
    clear_value.Format = format;
    if (bind_flag & BindFlag::kRtv)
    {
        clear_value.Color[0] = 0.0f;
        clear_value.Color[1] = 0.0f;
        clear_value.Color[2] = 0.0f;
        clear_value.Color[3] = 1.0f;
        p_clear_value = &clear_value;
    }
    else if (bind_flag & BindFlag::kDsv)
    {
        clear_value.DepthStencil.Depth = 1.0f;
        clear_value.DepthStencil.Stencil = 0;
        if (format == DXGI_FORMAT_R32_TYPELESS)
            clear_value.Format = DXGI_FORMAT_D32_FLOAT;
        p_clear_value = &clear_value;
    }

    if (bind_flag & BindFlag::kUav)
        p_clear_value = nullptr;

    ASSERT_SUCCEEDED(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &desc,
        res->state,
        p_clear_value,
        IID_PPV_ARGS(&res->default_res)));
    res->desc = desc;

    return res;
}

Resource::Ptr DX12Context::CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, uint32_t stride)
{
    if (buffer_size == 0)
        return DX12Resource::Ptr();

    DX12Resource::Ptr res = std::make_shared<DX12Resource>(*this);

    if (bind_flag & BindFlag::kCbv)
        buffer_size = (buffer_size + 255) & ~255;

    auto desc = CD3DX12_RESOURCE_DESC::Buffer(buffer_size);

    res->bind_flag = bind_flag;
    res->buffer_size = buffer_size;
    res->stride = stride;
    res->state = D3D12_RESOURCE_STATE_COMMON;

    if (bind_flag & BindFlag::kRtv)
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    if (bind_flag & BindFlag::kDsv)
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    if (bind_flag & BindFlag::kUav)
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    if (bind_flag & BindFlag::KAccelerationStructure)
        res->state = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

    if (bind_flag & BindFlag::kCbv)
    {
        res->state = D3D12_RESOURCE_STATE_GENERIC_READ;
        device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &desc,
            res->state,
            nullptr,
            IID_PPV_ARGS(&res->default_res));
    }
    else
    {
        device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &desc,
            res->state,
            nullptr,
            IID_PPV_ARGS(&res->default_res));
    }
    res->desc = desc;
    return res;
}

Resource::Ptr DX12Context::CreateSampler(const SamplerDesc & desc)
{
    DX12Resource::Ptr res = std::make_shared<DX12Resource>(*this);

    D3D12_SAMPLER_DESC& sampler_desc = res->sampler_desc;

    switch (desc.filter)
    {
    case SamplerFilter::kAnisotropic:
        sampler_desc.Filter = D3D12_FILTER_ANISOTROPIC;
        break;
    case SamplerFilter::kMinMagMipLinear:
        sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        break;
    case SamplerFilter::kComparisonMinMagMipLinear:
        sampler_desc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        break;
    }

    switch (desc.mode)
    {
    case SamplerTextureAddressMode::kWrap:
        sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        break;
    case SamplerTextureAddressMode::kClamp:
        sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        break;
    }

    switch (desc.func)
    {
    case SamplerComparisonFunc::kNever:
        sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        break;
    case SamplerComparisonFunc::kAlways:
        sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        break;
    case SamplerComparisonFunc::kLess:
        sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS;
        break;
    }

    sampler_desc.MinLOD = 0;
    sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
    sampler_desc.MaxAnisotropy = 1;

    return res;
}

Resource::Ptr DX12Context::CreateBottomLevelAS(const BufferDesc& vertex)
{
    return CreateBottomLevelAS(vertex, {});
}

Resource::Ptr DX12Context::CreateBottomLevelAS(const BufferDesc& vertex, const BufferDesc& index)
{
    auto vertex_res = std::static_pointer_cast<DX12Resource>(vertex.res);
    auto index_res = std::static_pointer_cast<DX12Resource>(index.res);

    D3D12_RAYTRACING_GEOMETRY_DESC geometry_desc = {};
    geometry_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometry_desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    ASSERT(!!vertex_res);

    auto vertex_stride = gli::detail::bits_per_pixel(vertex.format) / 8;
    geometry_desc.Triangles.VertexBuffer.StartAddress = vertex_res->default_res->GetGPUVirtualAddress() + vertex.offset * vertex_stride;
    geometry_desc.Triangles.VertexBuffer.StrideInBytes = vertex_stride;
    geometry_desc.Triangles.VertexFormat = static_cast<DXGI_FORMAT>(gli::dx().translate(vertex.format).DXGIFormat.DDS);
    geometry_desc.Triangles.VertexCount = vertex.count;
    if (index_res)
    {
        auto index_stride = gli::detail::bits_per_pixel(index.format) / 8;
        geometry_desc.Triangles.IndexBuffer = index_res->default_res->GetGPUVirtualAddress() + index.offset * index_stride;
        geometry_desc.Triangles.IndexFormat = static_cast<DXGI_FORMAT>(gli::dx().translate(index.format).DXGIFormat.DDS);
        geometry_desc.Triangles.IndexCount = index.count;
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.NumDescs = 1;
    inputs.pGeometryDescs = &geometry_desc;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
    device5->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

    auto scratch = std::static_pointer_cast<DX12Resource>(CreateBuffer(kUav, info.ScratchDataSizeInBytes, 0));
    auto result = std::static_pointer_cast<DX12Resource>(CreateBuffer(kUav | KAccelerationStructure, info.ResultDataMaxSizeInBytes, 0));

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC acceleration_structure_desc = {};
    acceleration_structure_desc.Inputs = inputs;
    acceleration_structure_desc.DestAccelerationStructureData = result->default_res->GetGPUVirtualAddress();
    acceleration_structure_desc.ScratchAccelerationStructureData = scratch->default_res->GetGPUVirtualAddress();
    command_list4->BuildRaytracingAccelerationStructure(&acceleration_structure_desc, 0, nullptr);

    D3D12_RESOURCE_BARRIER uav_barrier = {};
    uav_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    uav_barrier.UAV.pResource = result->default_res.Get();
    command_list4->ResourceBarrier(1, &uav_barrier);

    DX12Resource::Ptr res = std::make_shared<DX12Resource>(*this);
    res->as.scratch = scratch;
    res->as.result = result;
    res->default_res = result->default_res;

    return res;
}

Resource::Ptr DX12Context::CreateTopLevelAS(const std::vector<std::pair<Resource::Ptr, glm::mat4>>& geometry)
{
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    inputs.NumDescs = geometry.size();

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
    device5->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

    auto scratch = std::static_pointer_cast<DX12Resource>(CreateBuffer(kUav, info.ScratchDataSizeInBytes, 0));
    auto result = std::static_pointer_cast<DX12Resource>(CreateBuffer(kUav | KAccelerationStructure, info.ResultDataMaxSizeInBytes, 0));

    auto instance_desc_res = std::static_pointer_cast<DX12Resource>(CreateBuffer(0, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * geometry.size(), 0));
    auto& upload_res = instance_desc_res->GetUploadResource(0);
    if (!upload_res)
    {
        UINT64 buffer_size = GetRequiredIntermediateSize(instance_desc_res->default_res.Get(), 0, 1);
        device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(buffer_size),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&upload_res));
    }

    D3D12_RAYTRACING_INSTANCE_DESC* instance_desc = nullptr;
    ASSERT_SUCCEEDED(upload_res->Map(0, nullptr, reinterpret_cast<void**>(&instance_desc)));

    for (size_t i = 0; i < geometry.size(); ++i)
    {
        auto res = std::static_pointer_cast<DX12Resource>(geometry[i].first);

        instance_desc[i] = {};
        instance_desc[i].InstanceID = i;
        instance_desc[i].AccelerationStructure = res->default_res->GetGPUVirtualAddress();
        instance_desc[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        instance_desc[i].InstanceMask = 0xFF;
        memcpy(instance_desc[i].Transform, &geometry[i].second, sizeof(instance_desc->Transform));
    }

    upload_res->Unmap(0, nullptr);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC acceleration_structure_desc = {};
    acceleration_structure_desc.Inputs = inputs;
    acceleration_structure_desc.Inputs.InstanceDescs = upload_res->GetGPUVirtualAddress();
    acceleration_structure_desc.DestAccelerationStructureData = result->default_res->GetGPUVirtualAddress();
    acceleration_structure_desc.ScratchAccelerationStructureData = scratch->default_res->GetGPUVirtualAddress();
    command_list4->BuildRaytracingAccelerationStructure(&acceleration_structure_desc, 0, nullptr);

    D3D12_RESOURCE_BARRIER uav_barrier = {};
    uav_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    uav_barrier.UAV.pResource = result->default_res.Get();
    command_list4->ResourceBarrier(1, &uav_barrier);

    DX12Resource::Ptr res = std::make_shared<DX12Resource>(*this);
    res->as.scratch = scratch;
    res->as.result = result;
    res->as.instance_desc = instance_desc_res;
    res->default_res = result->default_res;

    return res;
}

void DX12Context::DispatchRays(uint32_t width, uint32_t height, uint32_t depth)
{
    m_current_program->ApplyBindings();
    m_current_program->DispatchRays(width, height, depth);
}

void DX12Context::UpdateSubresource(const Resource::Ptr& ires, uint32_t DstSubresource, const void * pSrcData, uint32_t SrcRowPitch, uint32_t SrcDepthPitch)
{
    auto res = std::static_pointer_cast<DX12Resource>(ires);

    if (res->bind_flag & BindFlag::kCbv)
    {
        CD3DX12_RANGE range(0, 0);
        char* cbvGPUAddress = nullptr;
        ASSERT_SUCCEEDED(res->default_res->Map(0, &range, reinterpret_cast<void**>(&cbvGPUAddress)));
        memcpy(cbvGPUAddress, pSrcData, res->buffer_size);
        res->default_res->Unmap(0, &range);
        return;
    }

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
    ResourceBarrier(res, D3D12_RESOURCE_STATE_COMMON);
}

void DX12Context::SetViewport(float width, float height)
{
    D3D12_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = (FLOAT)width;
    viewport.Height = (FLOAT)height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    command_list->RSSetViewports(1, &viewport);

    SetScissorRect(0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height));
}

void DX12Context::SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    D3D12_RECT rect = { left, top, right, bottom };
    command_list->RSSetScissorRects(1, &rect);
}

void DX12Context::IASetIndexBuffer(Resource::Ptr ires, gli::format Format)
{
    DXGI_FORMAT format = static_cast<DXGI_FORMAT>(gli::dx().translate(Format).DXGIFormat.DDS);

    auto res = std::static_pointer_cast<DX12Resource>(ires);
    D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
    indexBufferView.Format = format;
    indexBufferView.SizeInBytes = res->desc.Width;
    indexBufferView.BufferLocation = res->default_res->GetGPUVirtualAddress();
    command_list->IASetIndexBuffer(&indexBufferView);
    ResourceBarrier(res, D3D12_RESOURCE_STATE_INDEX_BUFFER);
}

void DX12Context::IASetVertexBuffer(uint32_t slot, Resource::Ptr ires)
{
    auto res = std::static_pointer_cast<DX12Resource>(ires);
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
    vertexBufferView.BufferLocation = res->default_res->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = res->desc.Width;
    vertexBufferView.StrideInBytes = res->stride;
    command_list->IASetVertexBuffers(slot, 1, &vertexBufferView);
    ResourceBarrier(res, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
}

void DX12Context::BeginEvent(const std::string& name)
{
    std::wstring wname = utf8_to_wstring(name);
    PIXBeginEvent(command_list.Get(), 0, wname.c_str());
}

void DX12Context::EndEvent()
{
    PIXEndEvent(command_list.Get());
}

void DX12Context::DrawIndexed(uint32_t IndexCount, uint32_t StartIndexLocation, int32_t BaseVertexLocation)
{
    m_current_program->ApplyBindings();
    command_list->DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
}

void DX12Context::Dispatch(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ)
{
    m_current_program->ApplyBindings();
    command_list->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

void DX12Context::InitBackBuffers()
{
    for (size_t i = 0; i < FrameCount; ++i)
    {
        DX12Resource::Ptr res = std::make_shared<DX12Resource>(*this);
        ComPtr<ID3D12Resource> back_buffer;
        ASSERT_SUCCEEDED(m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&back_buffer)));
        res->state = D3D12_RESOURCE_STATE_PRESENT;
        res->default_res = back_buffer;
        res->desc = back_buffer->GetDesc();
        m_back_buffers[i] = res;
    }
}

Resource::Ptr DX12Context::GetBackBuffer()
{
    return m_back_buffers[m_frame_index];   
}

void DX12Context::Present()
{
    if (m_is_open_render_pass)
    {
        command_list4->EndRenderPass();
        m_is_open_render_pass = false;
    }

    ResourceBarrier(std::static_pointer_cast<DX12Resource>(GetBackBuffer()), D3D12_RESOURCE_STATE_PRESENT);

    command_list->Close();
    ID3D12CommandList* ppCommandLists[] = { command_list.Get() };

    m_command_queue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    if (CurState::Instance().vsync)
    {
        ASSERT_SUCCEEDED(m_swap_chain->Present(1, 0));
    }
    else
    {
        ASSERT_SUCCEEDED(m_swap_chain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
    }

    MoveToNextFrame();

    ASSERT_SUCCEEDED(m_command_allocator[m_frame_index]->Reset());
    ASSERT_SUCCEEDED(command_list->Reset(m_command_allocator[m_frame_index].Get(), nullptr));

    m_deletion_queue[m_frame_index].clear();
    for (auto & x : m_created_program)
        x.get().OnPresent();
}

void DX12Context::OnDestroy()
{
    WaitForGpu();
}

void DX12Context::ResourceBarrier(const DX12Resource::Ptr& res, D3D12_RESOURCE_STATES state)
{
    ResourceBarrier(*res, state);
}

void DX12Context::ResourceBarrier(DX12Resource& res, D3D12_RESOURCE_STATES state)
{
    if (res.as.result)
        return;
    if (res.state != state)
        command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(res.default_res.Get(), res.state, state));
    res.state = state;
}

void DX12Context::UseProgram(DX12ProgramApi& program_api)
{
    if (m_current_program != &program_api && m_use_render_passes)
    {
        if (m_is_open_render_pass)
        {
            command_list4->EndRenderPass();
            m_is_open_render_pass = false;
        }
    }
    m_current_program = &program_api;
}

DescriptorPool& DX12Context::GetDescriptorPool()
{
    return *descriptor_pool;
}

void DX12Context::QueryOnDelete(ComPtr<IUnknown> res)
{
    m_deletion_queue[m_frame_index].push_back(res);
}

void DX12Context::ResizeBackBuffer(int width, int height)
{
    WaitForGpu();
    for (size_t i = 0; i < FrameCount; ++i)
    {
        m_fence_values[i] = m_fence_values[m_frame_index];
        m_deletion_queue[i].clear();
        m_back_buffers[i].reset();
    }
    DXGI_SWAP_CHAIN_DESC desc = {};
    m_swap_chain->GetDesc(&desc);
    ASSERT_SUCCEEDED(m_swap_chain->ResizeBuffers(FrameCount, width, height, desc.BufferDesc.Format, desc.Flags));
    m_frame_index = m_swap_chain->GetCurrentBackBufferIndex();
    InitBackBuffers();
}

void DX12Context::WaitForGpu()
{
    // Schedule a Signal command in the queue.
    ASSERT_SUCCEEDED(m_command_queue->Signal(m_fence.Get(), m_fence_values[m_frame_index]));

    // Wait until the fence has been processed.
    ASSERT_SUCCEEDED(m_fence->SetEventOnCompletion(m_fence_values[m_frame_index], m_fence_event));
    WaitForSingleObjectEx(m_fence_event, INFINITE, FALSE);

    // Increment the fence value for the current frame.
    ++m_fence_values[m_frame_index];
}

void DX12Context::MoveToNextFrame()
{
    // Schedule a Signal command in the queue.
    const uint64_t current_fence_value = m_fence_values[m_frame_index];
    ASSERT_SUCCEEDED(m_command_queue->Signal(m_fence.Get(), current_fence_value));

    // Update the frame index.
    m_frame_index = m_swap_chain->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is ready.
    if (m_fence->GetCompletedValue() < m_fence_values[m_frame_index])
    {
        ASSERT_SUCCEEDED(m_fence->SetEventOnCompletion(m_fence_values[m_frame_index], m_fence_event));
        WaitForSingleObjectEx(m_fence_event, INFINITE, FALSE);
    }

    // Set the fence value for the next frame.
    m_fence_values[m_frame_index] = current_fence_value + 1;
}
