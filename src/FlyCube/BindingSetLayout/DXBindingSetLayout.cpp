#include "BindingSetLayout/DXBindingSetLayout.h"

#include "Device/DXDevice.h"
#include "GPUDescriptorPool/DXGPUDescriptorPoolRange.h"
#include "Utilities/NotReached.h"

#include <directx/d3dx12.h>

#include <deque>

namespace {

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
    default:
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
    case ViewType::kByteAddressBuffer:
    case ViewType::kAccelerationStructure:
        return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    case ViewType::kRWTexture:
    case ViewType::kRWBuffer:
    case ViewType::kRWStructuredBuffer:
    case ViewType::kRWByteAddressBuffer:
        return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    case ViewType::kConstantBuffer:
        return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    case ViewType::kSampler:
        return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    default:
        NOTREACHED();
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
        NOTREACHED();
    }
}

using RootKey = std::pair<D3D12_DESCRIPTOR_HEAP_TYPE, ShaderType>;

RootKey GetRootKey(const BindKey& bind_key)
{
    D3D12_DESCRIPTOR_HEAP_TYPE heap_type = GetHeapType(bind_key.view_type);
    return { heap_type, bind_key.shader_type };
}

bool IsCompute(ShaderType shader_type)
{
    switch (shader_type) {
    case ShaderType::kCompute:
    case ShaderType::kLibrary:
        return true;
    default:
        return false;
    }
}

} // namespace

DXBindingSetLayout::DXBindingSetLayout(DXDevice& device, const BindingSetLayoutDesc& desc)
    : device_(device)
{
    std::vector<D3D12_ROOT_PARAMETER> root_parameters;
    std::vector<D3D12_ROOT_PARAMETER> root_constants;
    std::map<RootKey, std::vector<D3D12_DESCRIPTOR_RANGE>> descriptor_table_ranges;
    std::map<RootKey, size_t> descriptor_table_offset;
    std::deque<D3D12_DESCRIPTOR_RANGE> bindless_ranges;

    auto add_root_table = [&](ShaderType shader_type, size_t range_count, const D3D12_DESCRIPTOR_RANGE* ranges) {
        D3D12_ROOT_DESCRIPTOR_TABLE descriptor_table = {};
        descriptor_table.NumDescriptorRanges = range_count;
        descriptor_table.pDescriptorRanges = ranges;

        size_t root_param_index = root_parameters.size();
        D3D12_ROOT_PARAMETER& root_parameter = root_parameters.emplace_back();
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
        descriptor_tables_[root_param_index].heap_type = GetHeapType(view_type);
        descriptor_tables_[root_param_index].heap_offset = 0;
        descriptor_tables_[root_param_index].bindless = true;
        descriptor_tables_[root_param_index].is_compute = IsCompute(shader_type);
    };

    auto handle_bind_key = [&](const BindKey& bind_key) {
        RootKey root_key = GetRootKey(bind_key);
        D3D12_DESCRIPTOR_HEAP_TYPE heap_type = root_key.first;
        if (!descriptor_table_offset.contains(root_key)) {
            descriptor_table_offset[root_key] = heap_descs_[heap_type];
        }

        decltype(auto) layout = layout_[bind_key];
        layout.heap_type = heap_type;
        layout.heap_offset = heap_descs_[heap_type];

        decltype(auto) range = descriptor_table_ranges[root_key].emplace_back();
        range.RangeType = GetRangeType(bind_key.view_type);
        range.NumDescriptors = bind_key.count;
        range.BaseShaderRegister = bind_key.slot;
        range.RegisterSpace = bind_key.space;
        range.OffsetInDescriptorsFromTableStart = layout.heap_offset - descriptor_table_offset[root_key];

        heap_descs_[heap_type] += bind_key.count;
    };

    for (const auto& bind_key : desc.bind_keys) {
        if (bind_key.count == kBindlessCount) {
            add_bindless_range(bind_key.shader_type, bind_key.view_type, bind_key.slot, bind_key.space);
            continue;
        }

        handle_bind_key(bind_key);
    }

    auto add_root_constant = [&](const BindKey& bind_key, uint32_t num_constants) {
        size_t root_param_index = root_constants.size();
        D3D12_ROOT_PARAMETER& root_parameter = root_constants.emplace_back();
        root_parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        root_parameter.Constants.ShaderRegister = bind_key.slot;
        root_parameter.Constants.RegisterSpace = bind_key.space;
        root_parameter.Constants.Num32BitValues = num_constants;
        root_parameter.ShaderVisibility = GetVisibility(bind_key.shader_type);
        return root_param_index;
    };

    size_t root_cost = descriptor_table_ranges.size();
    std::map<RootKey, size_t> extra_descriptor_table_ranges;
    for (const auto& [bind_key, _] : desc.constants) {
        assert(bind_key.count == 1);
        RootKey root_key = GetRootKey(bind_key);
        if (!descriptor_table_ranges.contains(root_key)) {
            ++extra_descriptor_table_ranges[root_key];
        }
    }

    for (const auto& [bind_key, size] : desc.constants) {
        RootKey root_key = GetRootKey(bind_key);
        if (!descriptor_table_ranges.contains(root_key)) {
            if (--extra_descriptor_table_ranges[root_key] == 0) {
                extra_descriptor_table_ranges.erase(root_key);
            }
        }

        uint64_t num_constants = (size + 3) / 4;
        if (root_cost + num_constants + extra_descriptor_table_ranges.size() <= D3D12_MAX_ROOT_COST) {
            uint32_t root_param_index = add_root_constant(bind_key, num_constants);
            root_cost += num_constants;
            constants_layout_[bind_key] = { root_param_index, static_cast<uint32_t>(num_constants),
                                            IsCompute(bind_key.shader_type) };
        } else {
            if (!descriptor_table_ranges.contains(root_key)) {
                ++root_cost;
            }
            handle_bind_key(bind_key);
            fallback_constants_.push_back({ bind_key, size });
        }
    }
    assert(root_cost <= D3D12_MAX_ROOT_COST);

    for (const auto& ranges : descriptor_table_ranges) {
        size_t root_param_index = add_root_table(ranges.first.second, ranges.second.size(), ranges.second.data());
        descriptor_tables_[root_param_index].heap_type = ranges.first.first;
        descriptor_tables_[root_param_index].heap_offset = descriptor_table_offset[ranges.first];
        descriptor_tables_[root_param_index].is_compute = IsCompute(ranges.first.second);
    }

    for (auto& [bind_key, desc] : constants_layout_) {
        desc.root_param_index += root_parameters.size();
    }
    for (const auto& root_constant : root_constants) {
        root_parameters.push_back(root_constant);
    }

    D3D12_ROOT_SIGNATURE_FLAGS root_signature_flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                                                      D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                                                      D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

    CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc = {};
    root_signature_desc.Init(static_cast<uint32_t>(root_parameters.size()), root_parameters.data(), 0, nullptr,
                             root_signature_flags);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error_blob;
    CHECK_HRESULT(
        D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error_blob));
    CHECK_HRESULT(device.GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                                          IID_PPV_ARGS(&root_signature_)));
}

const std::map<D3D12_DESCRIPTOR_HEAP_TYPE, size_t>& DXBindingSetLayout::GetHeapDescs() const
{
    return heap_descs_;
}

const std::map<BindKey, BindingLayoutDesc>& DXBindingSetLayout::GetLayout() const
{
    return layout_;
}

const std::map<uint32_t, DescriptorTableDesc>& DXBindingSetLayout::GetDescriptorTables() const
{
    return descriptor_tables_;
}

const ComPtr<ID3D12RootSignature>& DXBindingSetLayout::GetRootSignature() const
{
    return root_signature_;
}

const std::map<BindKey, ConstantsLayoutDesc>& DXBindingSetLayout::GetConstantsLayout() const
{
    return constants_layout_;
}

const std::vector<BindingConstants>& DXBindingSetLayout::GetFallbackConstants() const
{
    return fallback_constants_;
}
