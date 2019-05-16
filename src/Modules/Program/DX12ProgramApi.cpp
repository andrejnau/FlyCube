#include "Program/DX12ProgramApi.h"
#include "Program/BufferLayout.h"
#include "Texture/DXGIFormatHelper.h"
#include <Utilities/State.h>
#include <Shader/DXCompiler.h>
#include <Shader/DXReflector.h>
#include <d3d12shader.h>
#include <dxc/DXIL/DxilConstants.h>
#include <Shader/DXCLoader.h>
#include <dxc/DxilContainer/DxilContainer.h>
#include <dxc/DxilContainer/DxilRuntimeReflection.inl>

static const WCHAR* kHitGroup = L"hit_group";

inline UINT Align(UINT size, UINT alignment)
{
    return (size + (alignment - 1)) & ~(alignment - 1);
}

class Subobject
{
public:
    const D3D12_STATE_SUBOBJECT& GetSubobject() const
    {
        return m_state_subobject;
    }

protected:
    D3D12_STATE_SUBOBJECT m_state_subobject = {};
};

struct ShaderInfo
{
    uint32_t max_attribute_size = 0;
    uint32_t max_payload_size = 0;
    std::vector<std::pair<std::wstring, hlsl::DXIL::ShaderKind>> entries;
};

ShaderInfo GetShaderInfo(ComPtr<ID3DBlob> blob)
{
    ShaderInfo res;

    DXCLoader loader;
    CComPtr<IDxcBlobEncoding> source;
    UINT shade_idx = 0;
    ASSERT_SUCCEEDED(loader.library->CreateBlobWithEncodingOnHeapCopy(blob->GetBufferPointer(), static_cast<UINT32>(blob->GetBufferSize()), CP_ACP, &source));
    ASSERT_SUCCEEDED(loader.reflection->Load(source));
    uint32_t part_count = 0;
    ASSERT_SUCCEEDED(loader.reflection->GetPartCount(&part_count));
    for (uint32_t i = 0; i < part_count; ++i)
    {
        uint32_t kind = 0;
        ASSERT_SUCCEEDED(loader.reflection->GetPartKind(i, &kind));
        if (kind == hlsl::DxilFourCC::DFCC_RuntimeData)
        {
            CComPtr<IDxcBlob> part_blob;
            loader.reflection->GetPartContent(i, &part_blob);
            hlsl::RDAT::DxilRuntimeData context;
            context.InitFromRDAT(part_blob->GetBufferPointer(), part_blob->GetBufferSize());
            FunctionTableReader* func_table_reader = context.GetFunctionTableReader();
            for (uint32_t j = 0; j < func_table_reader->GetNumFunctions(); ++j)
            {
                FunctionReader func_reader = func_table_reader->GetItem(j);
                std::wstring name = utf8_to_wstring(func_reader.GetUnmangledName());
                hlsl::DXIL::ShaderKind type = func_reader.GetShaderKind();
                res.entries.emplace_back(name, type);

                res.max_attribute_size = std::max(res.max_attribute_size, func_reader.GetAttributeSizeInBytes());
                res.max_payload_size = std::max(res.max_payload_size, func_reader.GetPayloadSizeInBytes());
            }
        }
    }

    return res;
}

std::vector<std::pair<std::wstring, hlsl::DXIL::ShaderKind>> GetEntries(ComPtr<ID3DBlob> blob)
{
    return GetShaderInfo(blob).entries;
}

class DxilLibrary : public Subobject
{
public:
    DxilLibrary(ComPtr<ID3DBlob> blob)
    {
        m_state_subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
        m_state_subobject.pDesc = &m_dxil_lib_desc;

        auto entries = GetEntries(blob);

        m_export_desc.resize(entries.size());
        m_export_name.resize(entries.size());

        m_dxil_lib_desc.DXILLibrary.pShaderBytecode = blob->GetBufferPointer();
        m_dxil_lib_desc.DXILLibrary.BytecodeLength = blob->GetBufferSize();
        m_dxil_lib_desc.NumExports = entries.size();
        m_dxil_lib_desc.pExports = m_export_desc.data();

        for (size_t i = 0; i < entries.size(); ++i)
        {
            m_export_name[i] = entries[i].first;
            m_export_desc[i].Name = m_export_name[i].c_str();
        }
    }

private:
    D3D12_DXIL_LIBRARY_DESC m_dxil_lib_desc = {};
    std::vector<D3D12_EXPORT_DESC> m_export_desc;
    std::vector<std::wstring> m_export_name;
};

class ExportAssociation : public Subobject
{
public:
    ExportAssociation(const std::vector<std::wstring>& exports, const D3D12_STATE_SUBOBJECT& subobject_to_associate)
    {
        for (auto& str : exports)
        {
            m_exports.emplace_back(str.c_str());
        }

        m_association.NumExports = m_exports.size();
        m_association.pExports = m_exports.data();
        m_association.pSubobjectToAssociate = &subobject_to_associate;

        m_state_subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
        m_state_subobject.pDesc = &m_association;
    }

private:
    std::vector<const wchar_t*> m_exports;
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION m_association = {};
};

class ShaderConfig : public Subobject
{
public:
    ShaderConfig(ComPtr<ID3DBlob> blob)
    {
        auto info = GetShaderInfo(blob);
        m_shader_config.MaxAttributeSizeInBytes = info.max_attribute_size;
        m_shader_config.MaxPayloadSizeInBytes = info.max_payload_size;
        m_state_subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
        m_state_subobject.pDesc = &m_shader_config;
    }

private:
    D3D12_RAYTRACING_SHADER_CONFIG m_shader_config = {};
};

class PipelineConfig : public Subobject
{
public:
    PipelineConfig(uint32_t max_trace_recursion_depth)
    {
        config.MaxTraceRecursionDepth = max_trace_recursion_depth;

        m_state_subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
        m_state_subobject.pDesc = &config;
    }

private:
    D3D12_RAYTRACING_PIPELINE_CONFIG config = {};
};

class HitProgram : public Subobject
{
public:
    HitProgram(const wchar_t* hit_group_name, const std::vector<std::pair<std::wstring, hlsl::DXIL::ShaderKind>>& entries)
    {
        for (size_t i = 0; i < entries.size(); ++i)
        {
            switch (entries[i].second)
            {
            case hlsl::DXIL::ShaderKind::Intersection:
                m_desc.IntersectionShaderImport = entries[i].first.c_str();
                break;
            case hlsl::DXIL::ShaderKind::AnyHit:
                m_desc.AnyHitShaderImport = entries[i].first.c_str();
                break;
            case hlsl::DXIL::ShaderKind::ClosestHit:
                m_desc.ClosestHitShaderImport = entries[i].first.c_str();
                break;
            }
        }
        
        m_desc.HitGroupExport = hit_group_name;

        m_state_subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
        m_state_subobject.pDesc = &m_desc;
    }

private:
    D3D12_HIT_GROUP_DESC m_desc = {};
};

DX12ProgramApi::DX12ProgramApi(DX12Context& context)
    : CommonProgramApi(context)
    , m_context(context)
    , m_view_creater(m_context, *this)
{
    m_pso_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    m_pso_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    CD3DX12_DEPTH_STENCIL_DESC depth_stencil_desc(D3D12_DEFAULT);
    depth_stencil_desc.DepthEnable = true;
    depth_stencil_desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depth_stencil_desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depth_stencil_desc.StencilEnable = FALSE;
    m_pso_desc.DepthStencilState = depth_stencil_desc;
}

void DX12ProgramApi::LinkProgram()
{
    ParseShaders();

    if (m_is_dxr)
    {
        CreateRtPipelineState();
        CreateShaderTable();
    }
}

void DX12ProgramApi::UseProgram()
{
    m_context.UseProgram(*this);
    m_changed_binding = true;
    m_context.command_list->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    SetRootSignature(m_root_signature.Get());
    if (m_current_pso)
        m_context.command_list->SetPipelineState(m_current_pso.Get());
    else if (m_state_object)
        m_context.command_list4->SetPipelineState1(m_state_object.Get());
}

void DX12ProgramApi::DispatchRays(uint32_t width, uint32_t height, uint32_t depth)
{
    m_raytrace_desc.Width = width;
    m_raytrace_desc.Height = height;
    m_raytrace_desc.Depth = depth;
    m_context.command_list4->DispatchRays(&m_raytrace_desc);
}

void DX12ProgramApi::CreateShaderTable()
{
    auto entries = GetEntries(m_blob_map[ShaderType::kLibrary]);

    std::vector<std::pair<std::wstring, std::reference_wrapper<D3D12_GPU_VIRTUAL_ADDRESS>>> shader_entries;
    bool has_hit_group = false;
    for (size_t i = 0; i < entries.size(); ++i)
    {
        switch (entries[i].second)
        {
        case hlsl::DXIL::ShaderKind::RayGeneration:
            shader_entries.emplace_back(entries[i].first, m_raytrace_desc.RayGenerationShaderRecord.StartAddress);
            break;
        case hlsl::DXIL::ShaderKind::Miss:
            shader_entries.emplace_back(entries[i].first, m_raytrace_desc.MissShaderTable.StartAddress);
            break;
        case hlsl::DXIL::ShaderKind::Intersection:
        case hlsl::DXIL::ShaderKind::AnyHit:
        case hlsl::DXIL::ShaderKind::ClosestHit:
            has_hit_group = true;
            break;
        }
    }

    if (has_hit_group)
    {
        shader_entries.emplace_back(kHitGroup, m_raytrace_desc.HitGroupTable.StartAddress);
    }

    size_t shader_table_entry_size = Align(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
    shader_table_entry_size = Align(shader_table_entry_size, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    m_context.device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(shader_table_entry_size * shader_entries.size()),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_shader_table)
    );

    uint8_t* shader_table_data = nullptr;
    ASSERT_SUCCEEDED(m_shader_table->Map(0, nullptr, reinterpret_cast<void**>(&shader_table_data)));
    ComPtr<ID3D12StateObjectProperties> state_ojbect_props;
    m_state_object.As(&state_ojbect_props);

    for (size_t i = 0; i < shader_entries.size(); ++i)
    {
        memcpy(shader_table_data + i * shader_table_entry_size, state_ojbect_props->GetShaderIdentifier(shader_entries[i].first.c_str()), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        shader_entries[i].second.get() = m_shader_table->GetGPUVirtualAddress() + i * shader_table_entry_size;
    }

    m_shader_table->Unmap(0, nullptr);

    m_raytrace_desc.RayGenerationShaderRecord.SizeInBytes = shader_table_entry_size;
    m_raytrace_desc.MissShaderTable.SizeInBytes = shader_table_entry_size;
    m_raytrace_desc.HitGroupTable.SizeInBytes = shader_table_entry_size;

    m_raytrace_desc.MissShaderTable.StrideInBytes = shader_table_entry_size;
    m_raytrace_desc.HitGroupTable.StrideInBytes = shader_table_entry_size;
}

void DX12ProgramApi::CompileShader(const ShaderBase& shader)
{
    auto blob = DXCompile(shader);
    m_blob_map[shader.type] = blob;
    D3D12_SHADER_BYTECODE ShaderBytecode = {};
    ShaderBytecode.BytecodeLength = blob->GetBufferSize();
    ShaderBytecode.pShaderBytecode = blob->GetBufferPointer();
    switch (shader.type)
    {
    case ShaderType::kVertex:
    {
        m_pso_desc.VS = ShaderBytecode;
        DXReflect(m_blob_map[ShaderType::kVertex]->GetBufferPointer(), m_blob_map[ShaderType::kVertex]->GetBufferSize(), IID_PPV_ARGS(&m_input_layout_reflector));
        m_input_layout = GetInputLayout(m_input_layout_reflector);
        break;
    }
    case ShaderType::kPixel:
        m_pso_desc.PS = ShaderBytecode;
        break;
    case ShaderType::kCompute:
        m_compute_pso_desc.CS = ShaderBytecode;
        m_is_compute = true;
        break;
    case ShaderType::kGeometry:
        m_pso_desc.GS = ShaderBytecode;
        break;
    case ShaderType::kLibrary:
        m_is_compute = true;
        m_is_dxr = true;
        break;
    }

    m_pso_desc_cache = true;
}

void DX12ProgramApi::ClearRenderTarget(uint32_t slot, const std::array<float, 4>& color)
{
    if (m_context.m_use_render_passes)
    {
        auto& clear_color = m_clear_cache.GetColor(slot);
        clear_color.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        memcpy(clear_color.Color, color.data(), sizeof(color));
        m_clear_cache.GetColorLoadOp(slot) = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
    }
    else
    {
        auto& view = FindView(ShaderType::kPixel, ResourceType::kRtv, slot);
        if (!view)
            return;
        m_context.command_list->ClearRenderTargetView(ConvertView(view)->GetCpuHandle(), color.data(), 0, nullptr);
    }
}

void DX12ProgramApi::ClearDepthStencil(UINT ClearFlags, FLOAT Depth, UINT8 Stencil)
{
    if (m_context.m_use_render_passes)
    {
        auto& clear_color = m_clear_cache.GetDepth();
        clear_color.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        clear_color.DepthStencil = { Depth, Stencil };
        m_clear_cache.GetDepthLoadOp() = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
    }
    else
    {
        auto& view = FindView(ShaderType::kPixel, ResourceType::kDsv, 0);
        if (!view)
            return;
        m_context.command_list->ClearDepthStencilView(ConvertView(view)->GetCpuHandle(), (D3D12_CLEAR_FLAGS)ClearFlags, Depth, Stencil, 0, nullptr);
    }
}

void DX12ProgramApi::SetRasterizeState(const RasterizerDesc& desc)
{
    switch (desc.fill_mode)
    {
    case FillMode::kWireframe:
        m_pso_desc_cache(m_pso_desc.RasterizerState.FillMode) = D3D12_FILL_MODE_WIREFRAME;
        break;
    case FillMode::kSolid:
        m_pso_desc_cache(m_pso_desc.RasterizerState.FillMode) = D3D12_FILL_MODE_SOLID;
        break;
    }

    switch (desc.cull_mode)
    {
    case CullMode::kNone:
        m_pso_desc_cache(m_pso_desc.RasterizerState.CullMode) = D3D12_CULL_MODE_NONE;
        break;
    case CullMode::kFront:
        m_pso_desc_cache(m_pso_desc.RasterizerState.CullMode) = D3D12_CULL_MODE_FRONT;
        break;
    case CullMode::kBack:
        m_pso_desc_cache(m_pso_desc.RasterizerState.CullMode) = D3D12_CULL_MODE_BACK;
        break;
    }

    m_pso_desc_cache(m_pso_desc.RasterizerState.DepthBias) = desc.DepthBias;
}

void DX12ProgramApi::SetBlendState(const BlendDesc& desc)
{
    auto& rt_desc = m_pso_desc.BlendState.RenderTarget[0];

    auto convert = [](Blend type)
    {
        switch (type)
        {
        case Blend::kZero:
            return D3D12_BLEND_ZERO;
        case Blend::kSrcAlpha:
            return D3D12_BLEND_SRC_ALPHA;
        case Blend::kInvSrcAlpha:
            return D3D12_BLEND_INV_SRC_ALPHA;
        }
        return static_cast<D3D12_BLEND>(0);
    };

    auto convert_op = [](BlendOp type)
    {
        switch (type)
        {
        case BlendOp::kAdd:
            return D3D12_BLEND_OP_ADD;
        }
        return static_cast<D3D12_BLEND_OP>(0);
    };

    m_pso_desc_cache(rt_desc.BlendEnable) = desc.blend_enable;
    m_pso_desc_cache(rt_desc.BlendOp) = convert_op(desc.blend_op);
    m_pso_desc_cache(rt_desc.SrcBlend) = convert(desc.blend_src);
    m_pso_desc_cache(rt_desc.DestBlend) = convert(desc.blend_dest);
    m_pso_desc_cache(rt_desc.BlendOpAlpha) = convert_op(desc.blend_op_alpha);
    m_pso_desc_cache(rt_desc.SrcBlendAlpha) = convert(desc.blend_src_alpha);
    m_pso_desc_cache(rt_desc.DestBlendAlpha) = convert(desc.blend_dest_apha);
    m_pso_desc_cache(rt_desc.RenderTargetWriteMask) = D3D12_COLOR_WRITE_ENABLE_ALL;
}

void DX12ProgramApi::SetDepthStencilState(const DepthStencilDesc& desc)
{
    m_pso_desc_cache(m_pso_desc.DepthStencilState.DepthEnable) = desc.depth_enable;
    if (desc.func == DepthComparison::kLess)
        m_pso_desc_cache(m_pso_desc.DepthStencilState.DepthFunc) = D3D12_COMPARISON_FUNC_LESS;
    else if (desc.func == DepthComparison::kLessEqual)
        m_pso_desc_cache(m_pso_desc.DepthStencilState.DepthFunc) = D3D12_COMPARISON_FUNC_LESS_EQUAL;
}

ShaderBlob DX12ProgramApi::GetBlobByType(ShaderType type) const
{
    auto it = m_blob_map.find(type);
    if (it == m_blob_map.end())
        return {};

    return { (uint8_t*)it->second->GetBufferPointer(), it->second->GetBufferSize() };
}

void DX12ProgramApi::OnAttachSRV(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const Resource::Ptr& ires)
{
    m_changed_binding = true;
    if (!ires)
        return;

    DX12Resource& res = static_cast<DX12Resource&>(*ires);
    if (type == ShaderType::kPixel)
        m_context.ResourceBarrier(res, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    else
        m_context.ResourceBarrier(res, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void DX12ProgramApi::OnAttachUAV(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const Resource::Ptr& ires)
{
    m_changed_binding = true;
    if (!ires)
        return;
    DX12Resource& res = static_cast<DX12Resource&>(*ires);
    m_context.ResourceBarrier(res, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

void DX12ProgramApi::OnAttachCBV(ShaderType type, uint32_t slot, const Resource::Ptr& ires)
{
    m_changed_binding = true;
    /* TODO: DX12Resource::default_res is upload buffer for CBV
    if (!ires)
        return;
    DX12Resource& res = static_cast<DX12Resource&>(*ires);
    m_context.ResourceBarrier(res, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);*/
}

DX12Resource::Ptr DX12ProgramApi::CreateCBuffer(size_t buffer_size)
{
    DX12Resource::Ptr res = std::make_shared<DX12Resource>(m_context);
    buffer_size = (buffer_size + 255) & ~255;
    auto desc = CD3DX12_RESOURCE_DESC::Buffer(buffer_size);
    m_context.device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&res->default_res));
    return res;
}

void DX12ProgramApi::OnAttachSampler(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires)
{
    m_changed_binding = true;
}

void DX12ProgramApi::OnAttachRTV(uint32_t slot, const ViewDesc& view_desc, const Resource::Ptr& ires)
{
    m_changed_om = true;
    DX12Resource& res = static_cast<DX12Resource&>(*ires);

    m_context.ResourceBarrier(res, D3D12_RESOURCE_STATE_RENDER_TARGET);

    D3D12_RESOURCE_DESC& desc = res.desc;
    DXGI_FORMAT format = desc.Format;

    ComPtr<ID3D12ShaderReflection> reflector;
    DXReflect(m_blob_map[ShaderType::kPixel]->GetBufferPointer(), m_blob_map[ShaderType::kPixel]->GetBufferSize(), IID_PPV_ARGS(&reflector));

    D3D12_SIGNATURE_PARAMETER_DESC res_desc = {};
    ASSERT_SUCCEEDED(reflector->GetOutputParameterDesc(slot, &res_desc));

    switch (res_desc.ComponentType)
    {
    case D3D_REGISTER_COMPONENT_FLOAT32:
        format = FloatFromTypeless(desc.Format);
        break;
    }

    if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
        format = DXGI_FORMAT_R32_FLOAT;

    m_pso_desc_cache(m_pso_desc.RTVFormats[slot]) = format;
    m_pso_desc_cache(m_pso_desc.SampleDesc.Count) = desc.SampleDesc.Count;
}

void DX12ProgramApi::OnAttachDSV(const ViewDesc& view_desc, const Resource::Ptr& ires)
{
    m_changed_om = true;
    DX12Resource& res = static_cast<DX12Resource&>(*ires);

    m_context.ResourceBarrier(res, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    auto desc = res.default_res->GetDesc();
    DXGI_FORMAT format = DepthStencilFromTypeless(desc.Format);

    m_pso_desc_cache(m_pso_desc.DSVFormat) = format;
    m_pso_desc_cache(m_pso_desc.SampleDesc.Count) = res.default_res->GetDesc().SampleDesc.Count;
}

void DX12ProgramApi::SetRootSignature(ID3D12RootSignature * pRootSignature)
{
    if (m_is_compute)
        m_context.command_list->SetComputeRootSignature(pRootSignature);
    else
        m_context.command_list->SetGraphicsRootSignature(pRootSignature);
}

void DX12ProgramApi::SetRootDescriptorTable(UINT RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor)
{
    if (RootParameterIndex == -1)
        return;
    if (m_is_compute)
        m_context.command_list->SetComputeRootDescriptorTable(RootParameterIndex, BaseDescriptor);
    else
        m_context.command_list->SetGraphicsRootDescriptorTable(RootParameterIndex, BaseDescriptor);
}

void DX12ProgramApi::SetRootConstantBufferView(UINT RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation)
{
    if (RootParameterIndex == -1)
        return;
    if (m_is_compute)
        m_context.command_list->SetComputeRootConstantBufferView(RootParameterIndex, BufferLocation);
    else
        m_context.command_list->SetGraphicsRootConstantBufferView(RootParameterIndex, BufferLocation);
}

std::vector<D3D12_INPUT_ELEMENT_DESC> DX12ProgramApi::GetInputLayout(ComPtr<ID3D12ShaderReflection> reflector)
{
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
    if (!m_pso_desc_cache)
        return;
    m_pso_desc_cache = false;

    m_pso_desc.InputLayout.NumElements = static_cast<UINT>(m_input_layout.size());
    m_pso_desc.InputLayout.pInputElementDescs = m_input_layout.data();
    m_pso_desc.pRootSignature = m_root_signature.Get();
    m_pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    m_pso_desc.SampleMask = UINT_MAX;
    m_pso_desc.NumRenderTargets = m_num_rtv;

    auto it = m_pso.find(m_pso_desc);
    if (it == m_pso.end())
    {
        ASSERT_SUCCEEDED(m_context.device->CreateGraphicsPipelineState(&m_pso_desc, IID_PPV_ARGS(&m_current_pso)));
        m_pso.emplace(std::piecewise_construct,
            std::forward_as_tuple(m_pso_desc),
            std::forward_as_tuple(m_current_pso));
    }
    else
    {
        m_current_pso = it->second;
    }

    m_context.command_list->SetPipelineState(m_current_pso.Get());
}

void DX12ProgramApi::CreateComputePSO()
{
    if (!m_current_pso)
    {
        m_compute_pso_desc.pRootSignature = m_root_signature.Get();
        ASSERT_SUCCEEDED(m_context.device->CreateComputePipelineState(&m_compute_pso_desc, IID_PPV_ARGS(&m_current_pso)));
    }
    m_context.command_list->SetPipelineState(m_current_pso.Get());
}

void DX12ProgramApi::CreateRtPipelineState()
{
    size_t max_size = 16;
    std::vector<D3D12_STATE_SUBOBJECT> subobjects;
    subobjects.reserve(max_size);

    auto entries = GetEntries(m_blob_map[ShaderType::kLibrary]);

    std::vector<std::wstring> shader_entries;
    std::vector<std::wstring> hit_group;
    for (size_t i = 0; i < entries.size(); ++i)
    {
        switch (entries[i].second)
        {
        case hlsl::DXIL::ShaderKind::RayGeneration:
        case hlsl::DXIL::ShaderKind::Miss:
            shader_entries.emplace_back(entries[i].first);
            break;
        case hlsl::DXIL::ShaderKind::Intersection:
        case hlsl::DXIL::ShaderKind::AnyHit:
        case hlsl::DXIL::ShaderKind::ClosestHit:
            hit_group.emplace_back(entries[i].first);
            break;
        }
    }

    if (!hit_group.empty())
        shader_entries.emplace_back(kHitGroup);

    DxilLibrary dxil_lib(m_blob_map[ShaderType::kLibrary]);
    subobjects.emplace_back(dxil_lib.GetSubobject());

    subobjects.emplace_back();
    D3D12_ROOT_SIGNATURE_DESC local_root_sign_desc = {};
    local_root_sign_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
    ComPtr<ID3D12RootSignature> local_root_sign = CreateRootSignature(local_root_sign_desc);
    D3D12_STATE_SUBOBJECT& local_root_sign_subobject = subobjects.back();
    local_root_sign_subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
    local_root_sign_subobject.pDesc = local_root_sign.GetAddressOf();

    subobjects.emplace_back();
    D3D12_STATE_SUBOBJECT& global_root_sign_subobject = subobjects.back();
    global_root_sign_subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
    global_root_sign_subobject.pDesc = m_root_signature.GetAddressOf();

    ExportAssociation local_root_association(shader_entries, local_root_sign_subobject);
    subobjects.emplace_back(local_root_association.GetSubobject());

    ShaderConfig shader_config(m_blob_map[ShaderType::kLibrary]);
    subobjects.emplace_back(shader_config.GetSubobject());

    ExportAssociation shader_config_association(shader_entries, subobjects.back());
    subobjects.emplace_back(shader_config_association.GetSubobject());

    PipelineConfig pipeline_config(1);
    subobjects.emplace_back(pipeline_config.GetSubobject());

    HitProgram hit_program(kHitGroup, entries);
    subobjects.emplace_back(hit_program.GetSubobject());

    ASSERT(subobjects.capacity() == max_size);

    D3D12_STATE_OBJECT_DESC desc = {};
    desc.NumSubobjects = subobjects.size();
    desc.pSubobjects = subobjects.data();
    desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

    ASSERT_SUCCEEDED(m_context.device5->CreateStateObject(&desc, IID_PPV_ARGS(&m_state_object)));
}

void DX12ProgramApi::CopyDescriptor(DescriptorHeapRange& dst_range, size_t dst_offset, const View::Ptr& view)
{
    if (view)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE& src_cpu_handle = static_cast<DX12View&>(*view).GetCpuHandle();
        dst_range.CopyCpuHandle(dst_offset, src_cpu_handle);
    }
}

DX12View* DX12ProgramApi::ConvertView(const View::Ptr& view)
{
    if (!view)
        return nullptr;
    return &static_cast<DX12View&>(*view);
}

View::Ptr DX12ProgramApi::CreateView(const BindKey& bind_key, const ViewDesc& view_desc, const Resource::Ptr& res)
{
    return m_view_creater.GetView(bind_key, view_desc, GetBindingName(bind_key), res);
}

void DX12ProgramApi::ApplyBindings()
{
    if (!m_is_dxr)
    {
        if (m_is_compute)
            CreateComputePSO();
        else
            CreateGraphicsPSO();
    }

    UpdateCBuffers();

    if (m_changed_om)
    {
        if (m_context.m_use_render_passes)
            BeginRenderPass();
        else
            OMSetRenderTargets();
        m_changed_om = false;
    }

    if (!m_changed_binding)
        return;
    m_changed_binding = false;

    std::map<D3D12_DESCRIPTOR_HEAP_TYPE, size_t> descriptor_requests = {
        { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_num_cbv + m_num_srv + m_num_uav },
        { D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, m_num_sampler }
    };
    std::map<D3D12_DESCRIPTOR_HEAP_TYPE, std::map<BindKey, View::Ptr>> descriptor_cache;
    for (auto& x : m_bound_resources)
    {
        const BindKey& bind_key = x.first;
        switch (bind_key.res_type)
        {
        case ResourceType::kCbv:
        case ResourceType::kSrv:
        case ResourceType::kUav:
            descriptor_cache[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV][bind_key] = x.second.view;
            break;
        case ResourceType::kSampler:
            descriptor_cache[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER][bind_key] = x.second.view;
            break;
        }
    }

    std::map<D3D12_DESCRIPTOR_HEAP_TYPE, std::pair<bool, std::reference_wrapper<DescriptorHeapRange>>> descriptor_ranges;
    std::vector<ID3D12DescriptorHeap*> descriptor_heaps(descriptor_cache.size());
    size_t descriptor_index = 0;
    for (auto& x : descriptor_cache)
    {
        auto it = m_heap_cache.find(x.second);
        bool initialized = true;
        if (it == m_heap_cache.end())
        {
            it = m_heap_cache.emplace(x.second, m_context.GetDescriptorPool().Allocate(x.first, descriptor_requests[x.first])).first;
            initialized = false;
        }
        DescriptorHeapRange& heap_range = it->second;
        descriptor_ranges.emplace(std::piecewise_construct,
            std::forward_as_tuple(x.first),
            std::forward_as_tuple(initialized, heap_range));
        if (heap_range.GetSize() > 0)
        {
            descriptor_heaps[descriptor_index++] = heap_range.GetHeap().Get();
        }
    }

    if (descriptor_heaps.size())
        m_context.command_list->SetDescriptorHeaps(descriptor_heaps.size(), descriptor_heaps.data());

    for (auto& x : m_bound_resources)
    {
        D3D12_DESCRIPTOR_RANGE_TYPE range_type;
        D3D12_DESCRIPTOR_HEAP_TYPE heap_type;
        const BindKey& bind_key = x.first;
        switch (bind_key.res_type)
        {
        case ResourceType::kSrv:
            range_type = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            heap_type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            break;
        case ResourceType::kUav:
            range_type = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
            heap_type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            break;
        case ResourceType::kCbv:
            range_type = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
            heap_type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            break;
        case ResourceType::kSampler:
            range_type = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            heap_type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
            break;
        case ResourceType::kRtv:
        case ResourceType::kDsv:
            continue;
        }

        auto& descriptor_range = descriptor_ranges.find(heap_type)->second;

        if (descriptor_range.first)
            continue;

        CopyDescriptor(descriptor_range.second.get(), m_binding_layout[{bind_key.shader_type, range_type}].table.heap_offset + bind_key.slot, x.second.view);
    }

    for (auto &x : m_binding_layout)
    {
        if (x.second.type == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
        {
            D3D12_DESCRIPTOR_HEAP_TYPE heap_type;
            switch (std::get<1>(x.first))
            {
            case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
                heap_type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
                break;
            default:
                heap_type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
                break;
            }
            if (!descriptor_ranges.count(heap_type))
                continue;
            std::reference_wrapper<DescriptorHeapRange> heap_range = descriptor_ranges.find(heap_type)->second.second;
            if (x.second.root_param_index != -1)
                SetRootDescriptorTable(x.second.root_param_index, heap_range.get().GetGpuHandle(x.second.table.root_param_heap_offset));
        }
        else if (x.second.type == D3D12_ROOT_PARAMETER_TYPE_CBV)
        {
            for (uint32_t slot = 0; slot < x.second.view.root_param_num; ++slot)
            {
                auto& shader_type = std::get<0>(x.first);
                BindKey bind_key = { m_program_id, shader_type, ResourceType::kCbv, slot };
                auto& ires = m_cbv_buffer.get()[bind_key][m_cbv_offset.get()[bind_key]];
                DX12Resource& res = static_cast<DX12Resource&>(*ires);
                SetRootConstantBufferView(x.second.root_param_index + slot, res.default_res->GetGPUVirtualAddress());
            }
        }
    }
}

ComPtr<ID3D12RootSignature> DX12ProgramApi::CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& desc)
{
    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error_blob;
    ComPtr<ID3D12RootSignature> root_signature;
    ASSERT_SUCCEEDED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error_blob),
        "%s", (char*)error_blob->GetBufferPointer());
    ASSERT_SUCCEEDED(m_context.device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&root_signature)));
    return root_signature;
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

    std::vector<D3D12_ROOT_PARAMETER> root_parameters;
    std::deque<std::array<D3D12_DESCRIPTOR_RANGE, 4>> descriptor_table_ranges;
    for (auto& shader_blob : m_blob_map)
    {
        uint32_t num_cbv = 0;
        uint32_t num_srv = 0;
        uint32_t num_uav = 0;
        uint32_t num_sampler = 0;

        ComPtr<ID3D12ShaderReflection> shader_reflector;
        DXReflect(shader_blob.second->GetBufferPointer(), shader_blob.second->GetBufferSize(), IID_PPV_ARGS(&shader_reflector));
        if (shader_reflector)
        {
            D3D12_SHADER_DESC desc = {};
            shader_reflector->GetDesc(&desc);
            for (UINT i = 0; i < desc.BoundResources; ++i)
            {
                D3D12_SHADER_INPUT_BIND_DESC res_desc = {};
                ASSERT_SUCCEEDED(shader_reflector->GetResourceBindingDesc(i, &res_desc));
                ResourceType res_type;
                switch (res_desc.Type)
                {
                case D3D_SIT_SAMPLER:
                    num_sampler = std::max<uint32_t>(num_sampler, res_desc.BindPoint + res_desc.BindCount);
                    res_type = ResourceType::kSampler;
                    break;
                case D3D_SIT_CBUFFER:
                    num_cbv = std::max<uint32_t>(num_cbv, res_desc.BindPoint + res_desc.BindCount);
                    res_type = ResourceType::kCbv;
                    break;
                case D3D_SIT_TBUFFER:
                case D3D_SIT_TEXTURE:
                case D3D_SIT_STRUCTURED:
                case D3D_SIT_BYTEADDRESS:
                case D3D_SIT_RTACCELERATIONSTRUCTURE:
                    num_srv = std::max<uint32_t>(num_srv, res_desc.BindPoint + res_desc.BindCount);
                    res_type = ResourceType::kSrv;
                    break;
                default:
                    num_uav = std::max<uint32_t>(num_uav, res_desc.BindPoint + res_desc.BindCount);
                    res_type = ResourceType::kUav;
                    break;
                }
            }

            if (shader_blob.first == ShaderType::kPixel)
            {
                for (UINT i = 0; i < desc.OutputParameters; ++i)
                {
                    D3D12_SIGNATURE_PARAMETER_DESC param_desc = {};
                    shader_reflector->GetOutputParameterDesc(i, &param_desc);
                    m_num_rtv = std::max<uint32_t>(m_num_rtv, param_desc.Register + 1);
                }
            }
        }
        else
        {
            ComPtr<ID3D12LibraryReflection> library_reflector;
            DXReflect(shader_blob.second->GetBufferPointer(), shader_blob.second->GetBufferSize(), IID_PPV_ARGS(&library_reflector));
            if (library_reflector)
            {
                D3D12_LIBRARY_DESC lib_desc = {};
                library_reflector->GetDesc(&lib_desc);
                for (int j = 0; j < lib_desc.FunctionCount; ++j)
                {
                    auto reflector = library_reflector->GetFunctionByIndex(j);
                    D3D12_FUNCTION_DESC desc = {};
                    reflector->GetDesc(&desc);
                    for (UINT i = 0; i < desc.BoundResources; ++i)
                    {
                        D3D12_SHADER_INPUT_BIND_DESC res_desc = {};
                        ASSERT_SUCCEEDED(reflector->GetResourceBindingDesc(i, &res_desc));
                        ResourceType res_type;
                        switch (res_desc.Type)
                        {
                        case D3D_SIT_SAMPLER:
                            num_sampler = std::max<uint32_t>(num_sampler, res_desc.BindPoint + res_desc.BindCount);
                            res_type = ResourceType::kSampler;
                            break;
                        case D3D_SIT_CBUFFER:
                            num_cbv = std::max<uint32_t>(num_cbv, res_desc.BindPoint + res_desc.BindCount);
                            res_type = ResourceType::kCbv;
                            break;
                        case D3D_SIT_TBUFFER:
                        case D3D_SIT_TEXTURE:
                        case D3D_SIT_STRUCTURED:
                        case D3D_SIT_BYTEADDRESS:
                        case D3D_SIT_RTACCELERATIONSTRUCTURE:
                            num_srv = std::max<uint32_t>(num_srv, res_desc.BindPoint + res_desc.BindCount);
                            res_type = ResourceType::kSrv;
                            break;
                        default:
                            num_uav = std::max<uint32_t>(num_uav, res_desc.BindPoint + res_desc.BindCount);
                            res_type = ResourceType::kUav;
                            break;
                        }
                    }
                }
            }
        }

        D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL;
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

        m_binding_layout[{shader_blob.first, D3D12_DESCRIPTOR_RANGE_TYPE_SRV}].type = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        m_binding_layout[{shader_blob.first, D3D12_DESCRIPTOR_RANGE_TYPE_SRV}].table.root_param_heap_offset = num_resources;
        m_binding_layout[{shader_blob.first, D3D12_DESCRIPTOR_RANGE_TYPE_SRV}].table.heap_offset = num_resources;

        m_binding_layout[{shader_blob.first, D3D12_DESCRIPTOR_RANGE_TYPE_UAV}].type = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        m_binding_layout[{shader_blob.first, D3D12_DESCRIPTOR_RANGE_TYPE_UAV}].table.root_param_heap_offset = num_resources;
        m_binding_layout[{shader_blob.first, D3D12_DESCRIPTOR_RANGE_TYPE_UAV}].table.heap_offset = num_resources + num_srv;

        if (m_use_cbv_table)
        {
            m_binding_layout[{shader_blob.first, D3D12_DESCRIPTOR_RANGE_TYPE_CBV}].type = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            m_binding_layout[{shader_blob.first, D3D12_DESCRIPTOR_RANGE_TYPE_CBV}].table.root_param_heap_offset = num_resources;
            m_binding_layout[{shader_blob.first, D3D12_DESCRIPTOR_RANGE_TYPE_CBV}].table.heap_offset = num_resources + num_srv + num_uav;
        }
        else
        {
            m_binding_layout[{shader_blob.first, D3D12_DESCRIPTOR_RANGE_TYPE_CBV}].type = D3D12_ROOT_PARAMETER_TYPE_CBV;
            m_binding_layout[{shader_blob.first, D3D12_DESCRIPTOR_RANGE_TYPE_CBV}].view.root_param_num = num_cbv;
        }

        m_binding_layout[{shader_blob.first, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER}].type = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        m_binding_layout[{shader_blob.first, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER}].table.root_param_heap_offset = num_samplers;
        m_binding_layout[{shader_blob.first, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER}].table.heap_offset = num_samplers;

        if (!num_srv)
            m_binding_layout[{shader_blob.first, D3D12_DESCRIPTOR_RANGE_TYPE_SRV}].root_param_index = -1;
        if (!num_uav)
            m_binding_layout[{shader_blob.first, D3D12_DESCRIPTOR_RANGE_TYPE_UAV}].root_param_index = -1;
        if (!num_cbv)
            m_binding_layout[{shader_blob.first, D3D12_DESCRIPTOR_RANGE_TYPE_CBV}].root_param_index = -1;
        if (!num_sampler)
            m_binding_layout[{shader_blob.first, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER}].root_param_index = -1;

        descriptor_table_ranges.emplace_back();
        uint32_t index = 0;

        if (((num_srv + num_uav) > 0) || (m_use_cbv_table && num_cbv > 0))
        {
            if (num_srv)
            {
                descriptor_table_ranges.back()[index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                descriptor_table_ranges.back()[index].NumDescriptors = num_srv;
                descriptor_table_ranges.back()[index].BaseShaderRegister = 0;
                descriptor_table_ranges.back()[index].RegisterSpace = 0;
                descriptor_table_ranges.back()[index].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                ++index;
            }

            if (num_uav)
            {
                descriptor_table_ranges.back()[index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                descriptor_table_ranges.back()[index].NumDescriptors = num_uav;
                descriptor_table_ranges.back()[index].BaseShaderRegister = 0;
                descriptor_table_ranges.back()[index].RegisterSpace = 0;
                descriptor_table_ranges.back()[index].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                ++index;
            }

            if (m_use_cbv_table && num_cbv)
            {
                descriptor_table_ranges.back()[index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                descriptor_table_ranges.back()[index].NumDescriptors = num_cbv;
                descriptor_table_ranges.back()[index].BaseShaderRegister = 0;
                descriptor_table_ranges.back()[index].RegisterSpace = 0;
                descriptor_table_ranges.back()[index].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                ++index;
            }

            D3D12_ROOT_DESCRIPTOR_TABLE descriptorTableTexture;
            descriptorTableTexture.NumDescriptorRanges = index;
            descriptorTableTexture.pDescriptorRanges = &descriptor_table_ranges.back()[0];

            m_binding_layout[{shader_blob.first, D3D12_DESCRIPTOR_RANGE_TYPE_SRV}].root_param_index = static_cast<uint32_t>(root_parameters.size());
            m_binding_layout[{shader_blob.first, D3D12_DESCRIPTOR_RANGE_TYPE_UAV}].root_param_index = static_cast<uint32_t>(root_parameters.size());
            if (m_use_cbv_table)
                m_binding_layout[{shader_blob.first, D3D12_DESCRIPTOR_RANGE_TYPE_CBV}].root_param_index = static_cast<uint32_t>(root_parameters.size());

            root_parameters.emplace_back();
            root_parameters.back().ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            root_parameters.back().DescriptorTable = descriptorTableTexture;
            root_parameters.back().ShaderVisibility = visibility;
        }

        if (!m_use_cbv_table && num_cbv > 0)
        {
            m_binding_layout[{shader_blob.first, D3D12_DESCRIPTOR_RANGE_TYPE_CBV}].root_param_index = static_cast<uint32_t>(root_parameters.size());

            for (uint32_t j = 0; j < num_cbv; ++j)
            {
                root_parameters.emplace_back();
                root_parameters.back().ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
                root_parameters.back().Descriptor.ShaderRegister = j;
                root_parameters.back().ShaderVisibility = visibility;
            }
        }
        
        if (num_sampler)
        {
            descriptor_table_ranges.back()[index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            descriptor_table_ranges.back()[index].NumDescriptors = num_sampler;
            descriptor_table_ranges.back()[index].BaseShaderRegister = 0;
            descriptor_table_ranges.back()[index].RegisterSpace = 0;
            descriptor_table_ranges.back()[index].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

            D3D12_ROOT_DESCRIPTOR_TABLE descriptorTableSampler;
            descriptorTableSampler.NumDescriptorRanges = 1;
            descriptorTableSampler.pDescriptorRanges = &descriptor_table_ranges.back()[index];

            m_binding_layout[{shader_blob.first, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER}].root_param_index = static_cast<uint32_t>(root_parameters.size());

            root_parameters.emplace_back();
            root_parameters.back().ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            root_parameters.back().DescriptorTable = descriptorTableSampler;
            root_parameters.back().ShaderVisibility = visibility;
        }

        num_resources += num_cbv + num_srv + num_uav;
        num_samplers += num_sampler;

        m_num_cbv += num_cbv;
        m_num_srv += num_srv;
        m_num_uav += num_uav;
        m_num_sampler += num_sampler;
    }

    D3D12_ROOT_SIGNATURE_FLAGS root_signature_flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

    CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc;
    root_signature_desc.Init(static_cast<UINT>(root_parameters.size()),
        root_parameters.data(),
        0,
        nullptr,
        root_signature_flags);

    m_root_signature = CreateRootSignature(root_signature_desc);
}

void DX12ProgramApi::OnPresent()
{
    m_cbv_offset.get().clear();
}

void DX12ProgramApi::OMSetRenderTargets()
{
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> om_rtv(m_num_rtv);
    for (uint32_t slot = 0; slot < m_num_rtv; ++slot)
    {
        auto& view = FindView(ShaderType::kPixel, ResourceType::kRtv, slot);
        if (view)
            om_rtv[slot] = ConvertView(view)->GetCpuHandle();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE om_dsv = {};
    D3D12_CPU_DESCRIPTOR_HANDLE* om_dsv_ptr = nullptr;
    auto view = FindView(ShaderType::kPixel, ResourceType::kDsv, 0);
    if (view)
    {
        om_dsv = ConvertView(view)->GetCpuHandle();
        om_dsv_ptr = &om_dsv;
    }
    m_context.command_list->OMSetRenderTargets(static_cast<UINT>(om_rtv.size()), om_rtv.data(), FALSE, om_dsv_ptr);
}

void DX12ProgramApi::BeginRenderPass()
{
    if (m_context.m_is_open_render_pass)
        m_context.command_list4->EndRenderPass();

    std::vector<D3D12_RENDER_PASS_RENDER_TARGET_DESC> om_rtv(m_num_rtv);
    for (uint32_t slot = 0; slot < m_num_rtv; ++slot)
    {
        auto& view = FindView(ShaderType::kPixel, ResourceType::kRtv, slot);
        if (view)
        {
            D3D12_RENDER_PASS_BEGINNING_ACCESS begin = { m_clear_cache.GetColorLoadOp(slot), m_clear_cache.GetColor(slot) };
            D3D12_RENDER_PASS_ENDING_ACCESS end = { D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE, {} };
            om_rtv[slot] = { ConvertView(view)->GetCpuHandle(), begin, end };
        }
    }

    D3D12_RENDER_PASS_DEPTH_STENCIL_DESC om_dsv = {};
    D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* om_dsv_ptr = nullptr;
    auto view = FindView(ShaderType::kPixel, ResourceType::kDsv, 0);
    if (view)
    {
        D3D12_RENDER_PASS_BEGINNING_ACCESS begin = { m_clear_cache.GetDepthLoadOp(), m_clear_cache.GetDepth() };
        D3D12_RENDER_PASS_ENDING_ACCESS end = { D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE, {} };
        om_dsv = { ConvertView(view)->GetCpuHandle(), begin, begin, end, end };
        om_dsv_ptr = &om_dsv;
    }
    m_context.command_list4->BeginRenderPass(static_cast<UINT>(om_rtv.size()), om_rtv.data(), om_dsv_ptr, D3D12_RENDER_PASS_FLAG_NONE);

    m_context.m_is_open_render_pass = true;
}
