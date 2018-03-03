#include "Program/DX12ProgramApi.h"

size_t GenId()
{
    static size_t id = 0;
    return ++id;
}

size_t GetBindingId(size_t program_id, ShaderType shader_type, ResourceType res_type, uint32_t slot)
{
    static std::map<std::tuple<size_t, ShaderType, ResourceType, uint32_t>, size_t> binding_id;
    auto it = binding_id.find({ program_id, shader_type, res_type, slot });
    if (it != binding_id.end())
        return it->second;

    return binding_id.emplace(std::piecewise_construct,
        std::forward_as_tuple(program_id, shader_type, res_type, slot),
        std::forward_as_tuple(GenId())).first->second;
}

DX12ProgramApi::DX12ProgramApi(DX12Context& context)
    : m_context(context)
    , m_program_id(GenId())
{
}

void DX12ProgramApi::SetMaxEvents(size_t count)
{
    m_context.descriptor_pool->ReqFrameDescription(ResourceType::kCbv, m_num_cbv * count);
    m_context.descriptor_pool->ReqFrameDescription(ResourceType::kSrv, m_num_srv * count);
    m_context.descriptor_pool->ReqFrameDescription(ResourceType::kUav, m_num_uav * count);
    m_context.descriptor_pool->ReqFrameDescription(ResourceType::kSampler, m_num_sampler * count);
}

void DX12ProgramApi::UseProgram()
{
    is_first_draw = true;

    draw_offset = 0;

    m_context.current_program = this;

    m_context.commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    SetRootSignature(rootSignature.Get());


    if (m_num_rtv)
    {
        D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
        heap_desc.NumDescriptors = m_num_rtv;
        heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        ASSERT_SUCCEEDED(m_context.device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&rtv_heap)));
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
        heap_desc.NumDescriptors = 1;
        heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        ASSERT_SUCCEEDED(m_context.device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&dsv_heap)));
    }
}

void DX12ProgramApi::OnCompileShader(ShaderType type, const ComPtr<ID3DBlob>& blob)
{
    m_blob_map[type] = blob;
    D3D12_SHADER_BYTECODE ShaderBytecode = {};
    ShaderBytecode.BytecodeLength = blob->GetBufferSize();
    ShaderBytecode.pShaderBytecode = blob->GetBufferPointer();
    switch (type)
    {
    case ShaderType::kVertex:
        psoDesc.VS = ShaderBytecode;
        break;
    case ShaderType::kPixel:
        psoDesc.PS = ShaderBytecode;
        break;
    case ShaderType::kCompute:
        compute_pso.CS = ShaderBytecode;
        break;
    case ShaderType::kGeometry:
        psoDesc.GS = ShaderBytecode;
        break;
    }

    change_pso_desc = true;
    ParseShaders();
}

void DX12ProgramApi::AttachSRV(ShaderType type, const std::string & name, uint32_t slot, const Resource::Ptr& res)
{
    auto it = m_ranges.find({ type, ResourceType::kSrv, slot });
    if (it != m_ranges.end())
        m_ranges.erase(it);

    if (!res)
        return;

    m_ranges.emplace(std::piecewise_construct,
        std::forward_as_tuple(type, ResourceType::kSrv, slot),
        std::forward_as_tuple(CreateSrv(type, name, slot, res)));
}

void DX12ProgramApi::AttachUAV(ShaderType type, const std::string & name, uint32_t slot, const Resource::Ptr& res)
{
    auto it = m_ranges.find({ type, ResourceType::kUav, slot });
    if (it != m_ranges.end())
        m_ranges.erase(it);

    if (!res)
        return;

    m_ranges.emplace(std::piecewise_construct,
        std::forward_as_tuple(type, ResourceType::kUav, slot),
        std::forward_as_tuple(CreateUAV(type, name, slot, res)));
}

void DX12ProgramApi::AttachSampler(ShaderType type, uint32_t slot, const SamplerDesc& desc)
{
    auto it = m_ranges.find({ type, ResourceType::kSampler, slot });
    if (it != m_ranges.end())
        m_ranges.erase(it);

    m_ranges.emplace(std::piecewise_construct,
        std::forward_as_tuple(type, ResourceType::kSampler, slot),
        std::forward_as_tuple(CreateSampler(type, slot, desc)));
}

void DX12ProgramApi::AttachCBuffer(ShaderType type, uint32_t slot, const Resource::Ptr& ires)
{
    auto it = m_ranges.find({ type, ResourceType::kCbv, slot });
    if (it != m_ranges.end())
        m_ranges.erase(it);

    if (!ires)
        return;

    m_ranges.emplace(std::piecewise_construct,
        std::forward_as_tuple(type, ResourceType::kCbv, slot),
        std::forward_as_tuple(CreateCBV(type, slot, ires)));
}

void DX12ProgramApi::UpdateData(ShaderType type, UINT slot, const Resource::Ptr & ires, const void * ptr)
{
    m_context.UpdateSubresource(ires, 0, ptr, 0, 0, ++ver[{type, slot}]);
    auto res = std::static_pointer_cast<DX12Resource>(ires);
    m_context.ResourceBarrier(res, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
}

Context & DX12ProgramApi::GetContext()
{
    return m_context;
}

DescriptorHeapRange DX12ProgramApi::CreateSrv(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires)
{
    auto res = std::static_pointer_cast<DX12Resource>(ires);

    if (type == ShaderType::kPixel)
        m_context.ResourceBarrier(res, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    else
        m_context.ResourceBarrier(res, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    bool is_created_view = m_context.descriptor_pool->HasDescriptor(ResourceType::kSrv, GetBindingId(m_program_id, type, ResourceType::kSrv, slot), ires);
    DescriptorHeapRange handle = m_context.descriptor_pool->GetDescriptor(ResourceType::kSrv, GetBindingId(m_program_id, type, ResourceType::kSrv, slot), ires);

    if (is_created_view)
        return handle;

    ComPtr<ID3D12ShaderReflection> reflector;
    D3DReflect(m_blob_map[type]->GetBufferPointer(), m_blob_map[type]->GetBufferSize(), IID_PPV_ARGS(&reflector));

    D3D12_SHADER_INPUT_BIND_DESC res_desc = {};
    ASSERT_SUCCEEDED(reflector->GetResourceBindingDescByName(name.c_str(), &res_desc));

    D3D12_RESOURCE_DESC desc = res->default_res->GetDesc();

    switch (res_desc.Dimension)
    {
    case D3D_SRV_DIMENSION_BUFFER:
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.Buffer.FirstElement = 0;
        srv_desc.Buffer.NumElements = desc.Width / res->stride;
        srv_desc.Buffer.StructureByteStride = res->stride;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        m_context.device->CreateShaderResourceView(res->default_res.Get(), &srv_desc, handle.GetCpuHandle());

        break;
    }
    case D3D_SRV_DIMENSION_TEXTURE2D:
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.Format = desc.Format;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Texture2D.MipLevels = desc.MipLevels;
        m_context.device->CreateShaderResourceView(res->default_res.Get(), &srv_desc, handle.GetCpuHandle());
        break;
    }
    case D3D_SRV_DIMENSION_TEXTURE2DMS:
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.Format = desc.Format;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
        m_context.device->CreateShaderResourceView(res->default_res.Get(), &srv_desc, handle.GetCpuHandle());
        break;
    }
    case D3D_SRV_DIMENSION_TEXTURECUBE:
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.Format = DXGI_FORMAT_R32_FLOAT; // TODO tex_dec.Format;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srv_desc.TextureCube.MipLevels = desc.MipLevels;
        m_context.device->CreateShaderResourceView(res->default_res.Get(), &srv_desc, handle.GetCpuHandle());
        break;
    }
    default:
        assert(false);
        break;
    }

    return handle;
}

DescriptorHeapRange DX12ProgramApi::CreateUAV(ShaderType type, const std::string & name, uint32_t slot, const Resource::Ptr & ires)
{
    auto res = std::static_pointer_cast<DX12Resource>(ires);

    bool is_created_view = m_context.descriptor_pool->HasDescriptor(ResourceType::kUav, GetBindingId(m_program_id, type, ResourceType::kUav, slot), ires);
    DescriptorHeapRange handle = m_context.descriptor_pool->GetDescriptor(ResourceType::kUav, GetBindingId(m_program_id, type, ResourceType::kUav, slot), ires);

    if (is_created_view)
        return handle;

    ComPtr<ID3D12ShaderReflection> reflector;
    D3DReflect(m_blob_map[type]->GetBufferPointer(), m_blob_map[type]->GetBufferSize(), IID_PPV_ARGS(&reflector));

    D3D12_SHADER_INPUT_BIND_DESC res_desc = {};
    ASSERT_SUCCEEDED(reflector->GetResourceBindingDescByName(name.c_str(), &res_desc));

    D3D12_RESOURCE_DESC desc = res->default_res->GetDesc();

    switch (res_desc.Dimension)
    {
    case D3D_SRV_DIMENSION_BUFFER:
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.Format = DXGI_FORMAT_UNKNOWN;
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uav_desc.Buffer.FirstElement = 0;
        uav_desc.Buffer.NumElements = desc.Width / res->stride;
        uav_desc.Buffer.StructureByteStride = res->stride;
        m_context.device->CreateUnorderedAccessView(res->default_res.Get(), nullptr, &uav_desc, handle.GetCpuHandle());

        break;
    }
    default:
        assert(false);
        break;
    }

    return handle;
}

DescriptorHeapRange DX12ProgramApi::CreateSampler(ShaderType type, uint32_t slot, const SamplerDesc& desc)
{
    DescriptorHeapRange handle = m_context.descriptor_pool->GetDescriptor(ResourceType::kSampler, GetBindingId(m_program_id, type, ResourceType::kSampler, slot), nullptr);

    D3D12_SAMPLER_DESC sampler_desc = {};

    switch (desc.filter)
    {
    case SamplerFilter::kAnisotropic:
        sampler_desc.Filter = D3D12_FILTER_ANISOTROPIC;
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
    case SamplerComparisonFunc::kLess:
        sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS;
        break;
    }

    sampler_desc.MinLOD = 0;
    sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
    sampler_desc.MaxAnisotropy = 1;

    m_context.device->CreateSampler(&sampler_desc, handle.GetCpuHandle());

    return handle;
}

DescriptorHeapRange DX12ProgramApi::CreateCBV(ShaderType type, uint32_t slot, const Resource::Ptr& ires)
{
    bool is_created_view = m_context.descriptor_pool->HasDescriptor(ResourceType::kCbv, GetBindingId(m_program_id, type, ResourceType::kCbv, slot), ires);
    DescriptorHeapRange handle = m_context.descriptor_pool->GetDescriptor(ResourceType::kCbv, GetBindingId(m_program_id, type, ResourceType::kCbv, slot), ires);

    auto res = std::static_pointer_cast<DX12Resource>(ires);

    m_context.ResourceBarrier(res, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    if (is_created_view)
        return handle;

    D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
    desc.BufferLocation = res->default_res->GetGPUVirtualAddress();
    desc.SizeInBytes = (res->default_res->GetDesc().Width + 255) & ~255;

    m_context.device->CreateConstantBufferView(&desc, handle.GetCpuHandle());

    return handle;
}

void DX12ProgramApi::SetRootSignature(ID3D12RootSignature * pRootSignature)
{
    if (m_blob_map.count(ShaderType::kCompute))
        m_context.commandList->SetComputeRootSignature(pRootSignature);
    else
        m_context.commandList->SetGraphicsRootSignature(pRootSignature);
}

void DX12ProgramApi::SetRootDescriptorTable(UINT RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor)
{
    if (RootParameterIndex == -1)
        return;
    if (m_blob_map.count(ShaderType::kCompute))
        m_context.commandList->SetComputeRootDescriptorTable(RootParameterIndex, BaseDescriptor);
    else
        m_context.commandList->SetGraphicsRootDescriptorTable(RootParameterIndex, BaseDescriptor);
}

std::vector<D3D12_INPUT_ELEMENT_DESC> DX12ProgramApi::GetInputLayout(ComPtr<ID3D12ShaderReflection> reflector)
{
    reflector.Get()->AddRef(); // fix me
    D3D12_SHADER_DESC shader_desc = {};
    reflector->GetDesc(&shader_desc);

    std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout_desc;
    for (UINT i = 0; i < shader_desc.InputParameters; ++i)
    {
        D3D12_SIGNATURE_PARAMETER_DESC param_desc = {};
        reflector->GetInputParameterDesc(i, &param_desc);

        D3D12_INPUT_ELEMENT_DESC layout = {};
        layout.SemanticName = param_desc.SemanticName;
        layout.SemanticIndex = param_desc.SemanticIndex;
        layout.InputSlot = i;
        layout.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        layout.InstanceDataStepRate = 0;

        if (param_desc.Mask == 1)
        {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                layout.Format = DXGI_FORMAT_R32_UINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                layout.Format = DXGI_FORMAT_R32_SINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                layout.Format = DXGI_FORMAT_R32_FLOAT;
        }
        else if (param_desc.Mask <= 3)
        {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                layout.Format = DXGI_FORMAT_R32G32_UINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                layout.Format = DXGI_FORMAT_R32G32_SINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                layout.Format = DXGI_FORMAT_R32G32_FLOAT;
        }
        else if (param_desc.Mask <= 7)
        {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                layout.Format = DXGI_FORMAT_R32G32B32_UINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                layout.Format = DXGI_FORMAT_R32G32B32_SINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                layout.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        }
        else if (param_desc.Mask <= 15)
        {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                layout.Format = DXGI_FORMAT_R32G32B32A32_UINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                layout.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                layout.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }
        input_layout_desc.push_back(layout);
    }

    return input_layout_desc;
}

void DX12ProgramApi::CreateGraphicsPSO()
{
    if (!change_pso_desc)
        return;

    change_pso_desc = false;

    ComPtr<ID3D12ShaderReflection> reflector;
    D3DReflect(m_blob_map[ShaderType::kVertex]->GetBufferPointer(), m_blob_map[ShaderType::kVertex]->GetBufferSize(), IID_PPV_ARGS(&reflector));

    auto input_layout_elements = GetInputLayout(reflector);
    D3D12_INPUT_LAYOUT_DESC input_layout = {};
    input_layout.NumElements = input_layout_elements.size();
    input_layout.pInputElementDescs = input_layout_elements.data();

    if (psoDesc.RTVFormats[0] != 0)
    {
        int b = 0;
    }

    psoDesc.RasterizerState = {};

    //if (psoDesc.DSVFormat == DXGI_FORMAT_D32_FLOAT)
    {
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        psoDesc.RasterizerState.DepthBias = 4096;
        psoDesc.RasterizerState.DepthClipEnable = false;
    }

    psoDesc.InputLayout = input_layout; // the structure describing our input layout
    psoDesc.pRootSignature = rootSignature.Get(); // the root signature that describes the input data this pso needs
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // type of topology we are drawing

    DXGI_SAMPLE_DESC sampleDesc = {};
    sampleDesc.Count = 1; // multisample count (no multisampling, so we just put 1, since we still need 1 sample)

    psoDesc.SampleDesc = sampleDesc; // must be the same sample description as the swapchain and depth/stencil buffer
    psoDesc.SampleMask = UINT_MAX; // sample mask has to do with multi-sampling. 0xffffffff means point sampling is done
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // a default rasterizer state.
    D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = blendDesc; // a default blent state.
    psoDesc.NumRenderTargets = m_num_rtv; // we are only binding one render target

    CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc(D3D12_DEFAULT);
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    depthStencilDesc.StencilEnable = FALSE;

    psoDesc.DepthStencilState = depthStencilDesc; // a default depth stencil state
    ASSERT_SUCCEEDED(m_context.device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateObject)));

}

void DX12ProgramApi::CreateComputePSO()
{

    compute_pso.pRootSignature = rootSignature.Get();
    ASSERT_SUCCEEDED(m_context.device->CreateComputePipelineState(&compute_pso, IID_PPV_ARGS(&pipelineStateObject)));
    m_context.commandList->SetPipelineState(pipelineStateObject.Get());
}

void DX12ProgramApi::ApplyBindings()
{
    if (m_blob_map.count(ShaderType::kCompute))
        CreateComputePSO();
    else
        CreateGraphicsPSO();

    m_context.commandList->SetPipelineState(pipelineStateObject.Get());

    DescriptorHeapRange res_heap = m_context.descriptor_pool->Allocate(ResourceType::kSrv, m_num_cbv + m_num_srv + m_num_uav);
    DescriptorHeapRange sampler_heap = m_context.descriptor_pool->Allocate(ResourceType::kSampler, m_num_sampler);

    ID3D12DescriptorHeap* descriptor_heaps[2] = {};
    size_t descriptor_count = 0;
    if (res_heap.GetSize())
        descriptor_heaps[descriptor_count++] = res_heap.GetHeap().Get();
    if (sampler_heap.GetSize())
        descriptor_heaps[descriptor_count++] = sampler_heap.GetHeap().Get();
    m_context.commandList->SetDescriptorHeaps(descriptor_count, descriptor_heaps);

    for (auto & x : m_ranges)
    {
        D3D12_DESCRIPTOR_RANGE_TYPE range_type;
        D3D12_DESCRIPTOR_HEAP_TYPE heap_type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        std::reference_wrapper<DescriptorHeapRange> heap_range = res_heap;
        switch (std::get<1>(x.first))
        {
        case ResourceType::kSrv:
            range_type = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            break;
        case ResourceType::kUav:
            range_type = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
            break;
        case ResourceType::kCbv:
            range_type = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
            break;
        case ResourceType::kSampler:
            range_type = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            heap_type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
            heap_range = std::ref(sampler_heap);
            break;
        }
        D3D12_CPU_DESCRIPTOR_HANDLE view_handle = x.second.GetCpuHandle();
        D3D12_CPU_DESCRIPTOR_HANDLE binding_handle = heap_range.get().GetCpuHandle(m_heap_offset_map[std::get<0>(x.first)][range_type] + std::get<2>(x.first));

        m_context.device->CopyDescriptors(
            1, &binding_handle, nullptr,
            1, &view_handle, nullptr,
            heap_type);
    }

    for (auto & x : m_root_param_map)
    {
        for (auto & y : x.second)
        {
            std::reference_wrapper<DescriptorHeapRange> heap_range = res_heap;
            switch (y.first)
            {
            case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
                heap_range = std::ref(sampler_heap);
                break;
            }

            SetRootDescriptorTable(y.second, heap_range.get().GetGpuHandle(m_root_param_start_heap_map[x.first][y.first]));
        }
    }
}

void DX12ProgramApi::ParseShaders()
{
    m_num_cbv = 0;
    m_num_srv = 0;
    m_num_uav = 0;
    m_num_rtv = 0;
    m_num_sampler = 0;

    size_t num_resources = 0;
    size_t num_samplers = 0;

    std::vector<D3D12_ROOT_PARAMETER> rootParameters;
    std::deque<std::array<D3D12_DESCRIPTOR_RANGE, 4>> descriptorTableRanges;
    for (auto& shader_blob : m_blob_map)
    {
        m_root_param_start_heap_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_SRV] = num_resources;
        m_root_param_start_heap_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_UAV] = num_resources;
        m_root_param_start_heap_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_CBV] = num_resources;
        m_root_param_start_heap_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER] = num_samplers;

        size_t num_cbv = 0;
        size_t num_srv = 0;
        size_t num_uav = 0;
        size_t num_sampler = 0;

        ComPtr<ID3D12ShaderReflection> reflector;
        D3DReflect(shader_blob.second->GetBufferPointer(), shader_blob.second->GetBufferSize(), IID_PPV_ARGS(&reflector));

        D3D12_SHADER_DESC desc = {};
        reflector->GetDesc(&desc);
        for (UINT i = 0; i < desc.BoundResources; ++i)
        {
            D3D12_SHADER_INPUT_BIND_DESC res_desc = {};
            ASSERT_SUCCEEDED(reflector->GetResourceBindingDesc(i, &res_desc));
            ResourceType res_type;
            switch (res_desc.Type)
            {
            case D3D_SIT_SAMPLER:
                num_sampler = std::max<size_t>(num_sampler, res_desc.BindPoint + res_desc.BindCount);
                res_type = ResourceType::kSampler;
                break;
            case D3D_SIT_CBUFFER:
                num_cbv = std::max<size_t>(num_cbv, res_desc.BindPoint + res_desc.BindCount);
                res_type = ResourceType::kCbv;
                break;
            case D3D_SIT_TBUFFER:
            case D3D_SIT_TEXTURE:
            case D3D_SIT_STRUCTURED:
            case D3D_SIT_BYTEADDRESS:
                num_srv = std::max<size_t>(num_srv, res_desc.BindPoint + res_desc.BindCount);
                res_type = ResourceType::kSrv;
                break;
            default:
                num_uav = std::max<size_t>(num_uav, res_desc.BindPoint + res_desc.BindCount);
                res_type = ResourceType::kUav;
                break;
            }
        }

        if (shader_blob.first == ShaderType::kPixel)
        {
            for (UINT i = 0; i < desc.OutputParameters; ++i)
            {
                D3D12_SIGNATURE_PARAMETER_DESC param_desc = {};
                reflector->GetOutputParameterDesc(i, &param_desc);
                m_num_rtv = std::max<size_t>(m_num_rtv, param_desc.Register + 1);
            }
        }

        D3D12_SHADER_VISIBILITY visibility;
        switch (shader_blob.first)
        {
        case ShaderType::kVertex:
            visibility = D3D12_SHADER_VISIBILITY_VERTEX;
            break;
        case ShaderType::kPixel:
            visibility = D3D12_SHADER_VISIBILITY_PIXEL;
            break;
        case ShaderType::kCompute:
            visibility = D3D12_SHADER_VISIBILITY_ALL;
            break;
        case ShaderType::kGeometry:
            visibility = D3D12_SHADER_VISIBILITY_GEOMETRY;
            break;
        }

        descriptorTableRanges.emplace_back();
        size_t index = 0;

        m_heap_offset_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_SRV] = num_resources;
        m_heap_offset_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_UAV] = num_resources + num_srv;
        m_heap_offset_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_CBV] = num_resources + num_srv + num_uav;
        m_heap_offset_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER] = num_samplers;

        if ((num_srv + num_uav + num_cbv) > 0)
        {
            if (num_srv)
            {
                descriptorTableRanges.back()[index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                descriptorTableRanges.back()[index].NumDescriptors = num_srv;
                descriptorTableRanges.back()[index].BaseShaderRegister = 0;
                descriptorTableRanges.back()[index].RegisterSpace = 0;
                descriptorTableRanges.back()[index].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                ++index;
            }

            if (num_uav)
            {
                descriptorTableRanges.back()[index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                descriptorTableRanges.back()[index].NumDescriptors = num_uav;
                descriptorTableRanges.back()[index].BaseShaderRegister = 0;
                descriptorTableRanges.back()[index].RegisterSpace = 0;
                descriptorTableRanges.back()[index].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                ++index;
            }

            if (num_cbv)
            {
                descriptorTableRanges.back()[index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                descriptorTableRanges.back()[index].NumDescriptors = num_cbv;
                descriptorTableRanges.back()[index].BaseShaderRegister = 0;
                descriptorTableRanges.back()[index].RegisterSpace = 0;
                descriptorTableRanges.back()[index].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                ++index;
            }

            D3D12_ROOT_DESCRIPTOR_TABLE descriptorTableTexture;
            descriptorTableTexture.NumDescriptorRanges = index;
            descriptorTableTexture.pDescriptorRanges = &descriptorTableRanges.back()[0];

            m_root_param_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_SRV] = rootParameters.size();
            m_root_param_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_UAV] = rootParameters.size();
            m_root_param_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_CBV] = rootParameters.size();

            rootParameters.emplace_back();
            rootParameters.back().ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            rootParameters.back().DescriptorTable = descriptorTableTexture;
            rootParameters.back().ShaderVisibility = visibility;
        }
        else
        {
            m_root_param_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_SRV] = -1;
            m_root_param_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_UAV] = -1;
            m_root_param_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_CBV] = -1;
        }

        if (num_sampler)
        {
            descriptorTableRanges.back()[index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            descriptorTableRanges.back()[index].NumDescriptors = num_sampler;
            descriptorTableRanges.back()[index].BaseShaderRegister = 0;
            descriptorTableRanges.back()[index].RegisterSpace = 0;
            descriptorTableRanges.back()[index].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

            D3D12_ROOT_DESCRIPTOR_TABLE descriptorTableSampler;
            descriptorTableSampler.NumDescriptorRanges = 1;
            descriptorTableSampler.pDescriptorRanges = &descriptorTableRanges.back()[index];

            m_root_param_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER] = rootParameters.size();

            rootParameters.emplace_back();
            rootParameters.back().ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            rootParameters.back().DescriptorTable = descriptorTableSampler;
            rootParameters.back().ShaderVisibility = visibility;
        }
        else
        {
            m_root_param_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER] = -1;
        }

        num_resources += num_cbv + num_srv + num_uav;
        num_samplers += num_sampler;

        m_num_cbv += num_cbv;
        m_num_srv += num_srv;
        m_num_uav += num_uav;
        m_num_sampler += num_sampler;
    }

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(rootParameters.size(),
        rootParameters.data(),
        0,
        nullptr,
        rootSignatureFlags);

    ID3DBlob* signature;
    ID3DBlob* errorBuff; // a buffer holding the error data if any
    ASSERT_SUCCEEDED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errorBuff),
        "%s", (char*)errorBuff->GetBufferPointer());
    ASSERT_SUCCEEDED(m_context.device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

}

void DX12ProgramApi::OnPresent()
{
    ver.clear();
}

void DX12ProgramApi::OMSetRenderTargets(std::vector<Resource::Ptr> rtv, Resource::Ptr dsv)
{
    size_t slot = 0;
    used_rtv = 0;
    for (auto& x : rtv)
    {
        ++used_rtv;
        CreateRTV(slot++, x);
    }

    if (dsv)
        use_dsv = true;
    CreateDSV(dsv);

    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvss;
    for (int i = 0; i < used_rtv; ++i)
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtv_heap->GetCPUDescriptorHandleForHeapStart(), i,
            m_context.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
        const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        m_context.commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        rtvss.push_back(rtvHandle);
    }
    D3D12_CPU_DESCRIPTOR_HANDLE om_dsv = {};
    if (dsv_heap && use_dsv)
        om_dsv = dsv_heap->GetCPUDescriptorHandleForHeapStart();
    m_context.commandList->OMSetRenderTargets(rtvss.size(), rtvss.data(), FALSE, &om_dsv);

    // clear the depth/stencil buffer
    m_context.commandList->ClearDepthStencilView(om_dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
}

void DX12ProgramApi::CreateRTV(uint32_t slot, const Resource::Ptr & ires)
{
    auto res = std::static_pointer_cast<DX12Resource>(ires);

    m_context.ResourceBarrier(res, D3D12_RESOURCE_STATE_RENDER_TARGET);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtv_heap->GetCPUDescriptorHandleForHeapStart(), slot,
        m_context.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

    if (psoDesc.RTVFormats[slot] != res->default_res->GetDesc().Format)
    {
        psoDesc.RTVFormats[slot] = res->default_res->GetDesc().Format;
        change_pso_desc = true;
    }

    m_context.device->CreateRenderTargetView(res->default_res.Get(), nullptr, rtvHandle);
}

void DX12ProgramApi::CreateDSV(const Resource::Ptr & ires)
{
    if (!ires)
        return;

    auto res = std::static_pointer_cast<DX12Resource>(ires);

    m_context.ResourceBarrier(res, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(dsv_heap->GetCPUDescriptorHandleForHeapStart(), 0,
        m_context.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV));

    auto desc = res->default_res->GetDesc();

    if (psoDesc.DSVFormat != res->default_res->GetDesc().Format)
    {
        change_pso_desc = true;
        psoDesc.DSVFormat = res->default_res->GetDesc().Format;
    }

    if (desc.Format == DXGI_FORMAT_R32_TYPELESS) // TODO
    {
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

        D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
        dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        dsv_desc.Texture2DArray.ArraySize = desc.DepthOrArraySize;
        m_context.device->CreateDepthStencilView(res->default_res.Get(), &dsv_desc, rtvHandle);

    }
    else
    {
        m_context.device->CreateDepthStencilView(res->default_res.Get(), nullptr, rtvHandle);
    }
}
