#include "BindingSetLayout/DXBindingSetLayout.h"

#include "Device/DXDevice.h"
#include "GPUDescriptorPool/DXGPUDescriptorPoolRange.h"
#include "Program/ProgramBase.h"

#include <directx/d3dx12.h>

#include <deque>
#include <stdexcept>

D3D12_SHADER_VISIBILITY GetVisibility(ShaderType shader_type)
{
    D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL;
    switch (shader_type) {
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
    case ShaderType::kAmplification:
        visibility = D3D12_SHADER_VISIBILITY_AMPLIFICATION;
        break;
    case ShaderType::kMesh:
        visibility = D3D12_SHADER_VISIBILITY_MESH;
        break;
    }
    return visibility;
}

D3D12_DESCRIPTOR_RANGE_TYPE GetRangeType(ViewType view_type)
{
    D3D12_DESCRIPTOR_RANGE_TYPE range_type;
    switch (view_type) {
    case ViewType::kTexture:
    case ViewType::kBuffer:
    case ViewType::kStructuredBuffer:
    case ViewType::kAccelerationStructure:
        return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    case ViewType::kRWTexture:
    case ViewType::kRWBuffer:
    case ViewType::kRWStructuredBuffer:
        return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    case ViewType::kConstantBuffer:
        return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    case ViewType::kSampler:
        return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    default:
        throw std::runtime_error("wrong view type");
    }
}

D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType(ViewType view_type)
{
    D3D12_DESCRIPTOR_RANGE_TYPE range_type;
    switch (GetRangeType(view_type)) {
    case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
    case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
    case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
        return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
        return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    default:
        throw std::runtime_error("wrong view type");
    }
}

DXBindingSetLayout::DXBindingSetLayout(DXDevice& device, const std::vector<BindKey>& descs)
    : m_device(device)
{
    std::vector<D3D12_ROOT_PARAMETER> root_parameters;
    using RootKey = std::pair<D3D12_DESCRIPTOR_HEAP_TYPE, ShaderType>;
    std::map<RootKey, std::vector<D3D12_DESCRIPTOR_RANGE>> descriptor_table_ranges;
    std::map<RootKey, size_t> descriptor_table_offset;
    std::deque<D3D12_DESCRIPTOR_RANGE> bindless_ranges;

    auto add_root_table = [&](ShaderType shader_type, size_t range_count, const D3D12_DESCRIPTOR_RANGE* ranges) {
        D3D12_ROOT_DESCRIPTOR_TABLE descriptor_table = {};
        descriptor_table.NumDescriptorRanges = range_count;
        descriptor_table.pDescriptorRanges = ranges;

        size_t root_param_index = root_parameters.size();
        decltype(auto) root_parameter = root_parameters.emplace_back();
        root_parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        root_parameter.DescriptorTable = descriptor_table;
        root_parameter.ShaderVisibility = GetVisibility(shader_type);
        return root_param_index;
    };

    auto add_bindless_range = [&](ShaderType shader_type, ViewType view_type, uint32_t base_slot, uint32_t space) {
        auto& descriptor_table_range = bindless_ranges.emplace_back();
        descriptor_table_range.RangeType = GetRangeType(view_type);
        descriptor_table_range.NumDescriptors = UINT_MAX;
        descriptor_table_range.BaseShaderRegister = base_slot;
        descriptor_table_range.RegisterSpace = space;
        size_t root_param_index = add_root_table(shader_type, 1, &descriptor_table_range);
        m_descriptor_tables[root_param_index].heap_type = GetHeapType(view_type);
        m_descriptor_tables[root_param_index].heap_offset = 0;
        m_descriptor_tables[root_param_index].bindless = true;
        switch (shader_type) {
        case ShaderType::kCompute:
        case ShaderType::kLibrary:
            m_descriptor_tables[root_param_index].is_compute = true;
            break;
        }
    };

    for (const auto& bind_key : descs) {
        if (bind_key.count == std::numeric_limits<uint32_t>::max()) {
            add_bindless_range(bind_key.shader_type, bind_key.view_type, bind_key.slot, bind_key.space);
            continue;
        }

        D3D12_DESCRIPTOR_HEAP_TYPE heap_type = GetHeapType(bind_key.view_type);
        decltype(auto) layout = m_layout[bind_key];
        layout.heap_type = heap_type;
        layout.heap_offset = m_heap_descs[heap_type];

        RootKey key = { heap_type, bind_key.shader_type };
        if (!descriptor_table_offset.count(key)) {
            descriptor_table_offset[key] = m_heap_descs[heap_type];
        }

        decltype(auto) range = descriptor_table_ranges[key].emplace_back();
        range.RangeType = GetRangeType(bind_key.view_type);
        range.NumDescriptors = bind_key.count;
        range.BaseShaderRegister = bind_key.slot;
        range.RegisterSpace = bind_key.space;
        range.OffsetInDescriptorsFromTableStart = layout.heap_offset - descriptor_table_offset[key];

        m_heap_descs[heap_type] += bind_key.count;
    }

    for (const auto& ranges : descriptor_table_ranges) {
        size_t root_param_index = add_root_table(ranges.first.second, ranges.second.size(), ranges.second.data());
        m_descriptor_tables[root_param_index].heap_type = ranges.first.first;
        m_descriptor_tables[root_param_index].heap_offset = descriptor_table_offset[ranges.first];
        switch (ranges.first.second) {
        case ShaderType::kCompute:
        case ShaderType::kLibrary:
            m_descriptor_tables[root_param_index].is_compute = true;
            break;
        }
    }

    D3D12_ROOT_SIGNATURE_FLAGS root_signature_flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                                                      D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                                                      D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

    CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc = {};
    root_signature_desc.Init(static_cast<uint32_t>(root_parameters.size()), root_parameters.data(), 0, nullptr,
                             root_signature_flags);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error_blob;
    ASSERT_SUCCEEDED(
        D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error_blob), "%s",
        static_cast<char*>(error_blob->GetBufferPointer()));
    ASSERT_SUCCEEDED(device.GetDevice()->CreateRootSignature(
        0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_root_signature)));
}

const std::map<D3D12_DESCRIPTOR_HEAP_TYPE, size_t>& DXBindingSetLayout::GetHeapDescs() const
{
    return m_heap_descs;
}

const std::map<BindKey, BindingLayout>& DXBindingSetLayout::GetLayout() const
{
    return m_layout;
}

const std::map<uint32_t, DescriptorTableDesc>& DXBindingSetLayout::GetDescriptorTables() const
{
    return m_descriptor_tables;
}

const ComPtr<ID3D12RootSignature>& DXBindingSetLayout::GetRootSignature() const
{
    return m_root_signature;
}
