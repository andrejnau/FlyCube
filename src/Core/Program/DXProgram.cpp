#include "Program/DXProgram.h"
#include <Device/DXDevice.h>
#include <Shader/DXReflector.h>
#include <Shader/DXShader.h>
#include <View/DXView.h>
#include <BindingSet/DXBindingSet.h>
#include <deque>
#include <set>
#include <d3dx12.h>

DXProgram::DXProgram(DXDevice& device, const std::vector<std::shared_ptr<Shader>>& shaders)
    : m_device(device)
{
    for (auto& shader : shaders)
    {
        m_shaders.emplace_back(std::static_pointer_cast<DXShader>(shader));
        m_shaders_by_type[shader->GetType()] = m_shaders.back();
    }

    std::vector<D3D12_ROOT_PARAMETER> root_parameters;
    std::deque<std::array<D3D12_DESCRIPTOR_RANGE, 4>> descriptor_table_ranges;
    for (auto& shader : m_shaders)
    {
        ShaderType shader_type = shader->GetType();
        if (shader_type == ShaderType::kCompute || shader_type == ShaderType::kLibrary)
            m_is_compute = true;
        ComPtr<ID3DBlob> shader_blob = shader->GetBlob();
        std::map<uint32_t, uint32_t> begin_cbv, begin_srv, begin_uav, begin_sampler;
        std::map<uint32_t, uint32_t> end_cbv, end_srv, end_uav, end_sampler;
        std::set<uint32_t> spaces;

        auto update_start = [&](std::map<uint32_t, uint32_t>& start, uint32_t space, uint32_t slot)
        {
            if (!start.count(space) || start[space] > slot)
                start[space] = slot;
        };

        ComPtr<ID3D12ShaderReflection> shader_reflector;
        DXReflect(shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(), IID_PPV_ARGS(&shader_reflector));
        if (shader_reflector)
        {
            D3D12_SHADER_DESC desc = {};
            shader_reflector->GetDesc(&desc);
            for (uint32_t i = 0; i < desc.BoundResources; ++i)
            {
                D3D12_SHADER_INPUT_BIND_DESC res_desc = {};
                ASSERT_SUCCEEDED(shader_reflector->GetResourceBindingDesc(i, &res_desc));
                spaces.insert(res_desc.Space);
                ViewType view_type;
                switch (res_desc.Type)
                {
                case D3D_SIT_SAMPLER:
                    update_start(begin_sampler, res_desc.Space, res_desc.BindPoint);
                    end_sampler[res_desc.Space] = std::max<uint32_t>(end_sampler[res_desc.Space], res_desc.BindPoint + res_desc.BindCount);
                    view_type = ViewType::kSampler;
                    break;
                case D3D_SIT_CBUFFER:
                    update_start(begin_cbv, res_desc.Space, res_desc.BindPoint);
                    end_cbv[res_desc.Space] = std::max<uint32_t>(end_cbv[res_desc.Space], res_desc.BindPoint + res_desc.BindCount);
                    view_type = ViewType::kConstantBuffer;
                    break;
                case D3D_SIT_TBUFFER:
                case D3D_SIT_TEXTURE:
                case D3D_SIT_STRUCTURED:
                case D3D_SIT_BYTEADDRESS:
                case D3D_SIT_RTACCELERATIONSTRUCTURE:
                    update_start(begin_srv, res_desc.Space, res_desc.BindPoint);
                    end_srv[res_desc.Space] = std::max<uint32_t>(end_srv[res_desc.Space], res_desc.BindPoint + res_desc.BindCount);
                    view_type = ViewType::kShaderResource;
                    break;
                default:
                    update_start(begin_uav, res_desc.Space, res_desc.BindPoint);
                    end_uav[res_desc.Space] = std::max<uint32_t>(end_uav[res_desc.Space], res_desc.BindPoint + res_desc.BindCount);
                    view_type = ViewType::kUnorderedAccess;
                    break;
                }

                m_bind_to_slot[{shader_type, view_type, res_desc.Name }] = { res_desc.BindPoint,  res_desc.Space };
            }
        }
        else
        {
            ComPtr<ID3D12LibraryReflection> library_reflector;
            DXReflect(shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(), IID_PPV_ARGS(&library_reflector));
            if (library_reflector)
            {
                D3D12_LIBRARY_DESC lib_desc = {};
                library_reflector->GetDesc(&lib_desc);
                for (uint32_t j = 0; j < lib_desc.FunctionCount; ++j)
                {
                    auto reflector = library_reflector->GetFunctionByIndex(j);
                    D3D12_FUNCTION_DESC desc = {};
                    reflector->GetDesc(&desc);
                    for (uint32_t i = 0; i < desc.BoundResources; ++i)
                    {
                        D3D12_SHADER_INPUT_BIND_DESC res_desc = {};
                        ASSERT_SUCCEEDED(reflector->GetResourceBindingDesc(i, &res_desc));
                        spaces.insert(res_desc.Space);
                        ViewType view_type;
                        switch (res_desc.Type)
                        {
                        case D3D_SIT_SAMPLER:
                            update_start(begin_sampler, res_desc.Space, res_desc.BindPoint);
                            end_sampler[res_desc.Space] = std::max<uint32_t>(end_sampler[res_desc.Space], res_desc.BindPoint + res_desc.BindCount);
                            view_type = ViewType::kSampler;
                            break;
                        case D3D_SIT_CBUFFER:
                            update_start(begin_cbv, res_desc.Space, res_desc.BindPoint);
                            end_cbv[res_desc.Space] = std::max<uint32_t>(end_cbv[res_desc.Space], res_desc.BindPoint + res_desc.BindCount);
                            view_type = ViewType::kConstantBuffer;
                            break;
                        case D3D_SIT_TBUFFER:
                        case D3D_SIT_TEXTURE:
                        case D3D_SIT_STRUCTURED:
                        case D3D_SIT_BYTEADDRESS:
                        case D3D_SIT_RTACCELERATIONSTRUCTURE:
                            update_start(begin_srv, res_desc.Space, res_desc.BindPoint);
                            end_srv[res_desc.Space] = std::max<uint32_t>(end_srv[res_desc.Space], res_desc.BindPoint + res_desc.BindCount);
                            view_type = ViewType::kShaderResource;
                            break;
                        default:
                            update_start(begin_uav, res_desc.Space, res_desc.BindPoint);
                            end_uav[res_desc.Space] = std::max<uint32_t>(end_uav[res_desc.Space], res_desc.BindPoint + res_desc.BindCount);
                            view_type = ViewType::kUnorderedAccess;
                            break;
                        }
                        m_bind_to_slot[{shader_type, view_type, res_desc.Name }] = { res_desc.BindPoint,  res_desc.Space };
                    }
                }
            }
        }

        D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL;
        switch (shader_type)
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

        for (uint32_t space : spaces)
        {
            uint32_t num_cbv = end_cbv[space] - begin_cbv[space];
            uint32_t num_srv = end_srv[space] - begin_srv[space];
            uint32_t num_uav = end_uav[space] - begin_uav[space];
            uint32_t num_sampler = end_sampler[space] - begin_sampler[space];

            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, space}].type = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, space}].table.root_param_heap_offset = m_num_resources;
            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, space}].table.heap_offset = m_num_resources;
            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, space}].table.start = begin_srv[space];

            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, space}].type = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, space}].table.root_param_heap_offset = m_num_resources;
            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, space}].table.heap_offset = m_num_resources + num_srv;
            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, space}].table.start = begin_uav[space];

            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, space}].type = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, space}].table.root_param_heap_offset = m_num_resources;
            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, space}].table.heap_offset = m_num_resources + num_srv + num_uav;
            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, space}].table.start = begin_cbv[space];

            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, space}].type = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, space}].table.root_param_heap_offset = m_num_samplers;
            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, space}].table.heap_offset = m_num_samplers;
            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, space}].table.start = begin_sampler[space];

            if (num_srv == 0)
                m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, space}].root_param_index = -1;
            if (num_uav == 0)
                m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, space}].root_param_index = -1;
            if (num_cbv == 0)
                m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, space}].root_param_index = -1;
            if (num_sampler == 0)
                m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, space}].root_param_index = -1;

            descriptor_table_ranges.emplace_back();
            uint32_t index = 0;

            if (num_srv + num_uav + num_cbv > 0)
            {
                if (num_srv)
                {
                    descriptor_table_ranges.back()[index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                    descriptor_table_ranges.back()[index].NumDescriptors = num_srv;
                    descriptor_table_ranges.back()[index].BaseShaderRegister = begin_srv[space];
                    descriptor_table_ranges.back()[index].RegisterSpace = space;
                    descriptor_table_ranges.back()[index].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                    ++index;
                }

                if (num_uav)
                {
                    descriptor_table_ranges.back()[index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                    descriptor_table_ranges.back()[index].NumDescriptors = num_uav;
                    descriptor_table_ranges.back()[index].BaseShaderRegister = begin_uav[space];
                    descriptor_table_ranges.back()[index].RegisterSpace = space;
                    descriptor_table_ranges.back()[index].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                    ++index;
                }

                if (num_cbv)
                {
                    descriptor_table_ranges.back()[index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                    descriptor_table_ranges.back()[index].NumDescriptors = num_cbv;
                    descriptor_table_ranges.back()[index].BaseShaderRegister = begin_cbv[space];
                    descriptor_table_ranges.back()[index].RegisterSpace = space;
                    descriptor_table_ranges.back()[index].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                    ++index;
                }

                D3D12_ROOT_DESCRIPTOR_TABLE descriptor_table = {};
                descriptor_table.NumDescriptorRanges = index;
                descriptor_table.pDescriptorRanges = &descriptor_table_ranges.back()[0];

                m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, space}].root_param_index = static_cast<uint32_t>(root_parameters.size());
                m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, space}].root_param_index = static_cast<uint32_t>(root_parameters.size());
                m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, space}].root_param_index = static_cast<uint32_t>(root_parameters.size());

                root_parameters.emplace_back();
                root_parameters.back().ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                root_parameters.back().DescriptorTable = descriptor_table;
                root_parameters.back().ShaderVisibility = visibility;
            }

            if (num_sampler)
            {
                descriptor_table_ranges.back()[index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
                descriptor_table_ranges.back()[index].NumDescriptors = num_sampler;
                descriptor_table_ranges.back()[index].BaseShaderRegister = begin_sampler[space];
                descriptor_table_ranges.back()[index].RegisterSpace = space;
                descriptor_table_ranges.back()[index].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

                D3D12_ROOT_DESCRIPTOR_TABLE descriptor_table;
                descriptor_table.NumDescriptorRanges = 1;
                descriptor_table.pDescriptorRanges = &descriptor_table_ranges.back()[index];

                m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, space}].root_param_index = static_cast<uint32_t>(root_parameters.size());

                root_parameters.emplace_back();
                root_parameters.back().ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                root_parameters.back().DescriptorTable = descriptor_table;
                root_parameters.back().ShaderVisibility = visibility;
            }

            m_num_resources += num_srv + num_uav + num_cbv;
            m_num_samplers += num_sampler;
        }
    }

    D3D12_ROOT_SIGNATURE_FLAGS root_signature_flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

    CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc;
    root_signature_desc.Init(static_cast<uint32_t>(root_parameters.size()),
        root_parameters.data(),
        0,
        nullptr,
        root_signature_flags);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error_blob;
    ASSERT_SUCCEEDED(D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error_blob),
        "%s", static_cast<char*>(error_blob->GetBufferPointer()));
    ASSERT_SUCCEEDED(device.GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_root_signature)));
}

void CopyDescriptor(DXGPUDescriptorPoolRange& dst_range, size_t dst_offset, const std::shared_ptr<View>& view)
{
    if (view)
    {
        decltype(auto) src_cpu_handle = view->As<DXView>().GetHandle();
        dst_range.CopyCpuHandle(dst_offset, src_cpu_handle);
    }
}

size_t DXProgram::HeapSizeByType(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    switch (type)
    {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        return m_num_resources;
    case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
        return m_num_samplers;
    default:
        assert(false);
        return 0;
    }
}

bool DXProgram::HasBinding(ShaderType shader, ViewType type, std::string name) const
{
    return m_bind_to_slot.count({ shader, type, name });
}

std::shared_ptr<BindingSet> DXProgram::CreateBindingSetImpl(const BindingsKey& bindings)
{
    BindingsByHeap bindings_by_heap;
    for (const auto& id : bindings)
    {
        const auto& desc = m_bindings[id];
        D3D12_DESCRIPTOR_HEAP_TYPE heap_type;
        switch (desc.type)
        {
        case ViewType::kConstantBuffer:
        case ViewType::kShaderResource:
        case ViewType::kUnorderedAccess:
            heap_type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            break;
        case ViewType::kSampler:
            heap_type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
            break;
        default:
            assert(false);
        }
        bindings_by_heap[heap_type].insert(id);
    }

    std::map<D3D12_DESCRIPTOR_HEAP_TYPE, std::shared_ptr<DXGPUDescriptorPoolRange>> descriptor_ranges;
    for (auto& x : bindings_by_heap)
    {
        std::shared_ptr<DXGPUDescriptorPoolRange> heap_range;
        auto it = m_heap_cache.find(x.second);
        if (it == m_heap_cache.end() || it->second.expired())
        {
            if (it != m_heap_cache.end())
                m_heap_cache.erase(it);
            heap_range = std::make_shared<DXGPUDescriptorPoolRange>(m_device.GetGPUDescriptorPool().Allocate(x.first, HeapSizeByType(x.first)));
            m_heap_cache.emplace(x.second, heap_range);
            for (auto& id : x.second)
            {
                auto& desc = m_bindings[id];
                DXBindKey bind_key = { desc.shader, desc.type, desc.name };
                D3D12_DESCRIPTOR_RANGE_TYPE range_type;
                D3D12_DESCRIPTOR_HEAP_TYPE heap_type;
                switch (desc.type)
                {
                case ViewType::kShaderResource:
                    range_type = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                    heap_type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
                    break;
                case ViewType::kUnorderedAccess:
                    range_type = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                    heap_type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
                    break;
                case ViewType::kConstantBuffer:
                    range_type = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                    heap_type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
                    break;
                case ViewType::kSampler:
                    range_type = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
                    heap_type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
                    break;
                case ViewType::kRenderTarget:
                case ViewType::kDepthStencil:
                    continue;
                }

                size_t slot = m_bind_to_slot.at(bind_key).first;
                size_t space = m_bind_to_slot.at(bind_key).second;
                auto& table = m_binding_layout.at({ bind_key.shader, range_type, space }).table;
                CopyDescriptor(*heap_range, table.heap_offset + (slot - table.start), desc.view);
            }
        }
        else
        {
            heap_range = it->second.lock();
        }
        descriptor_ranges.emplace(std::piecewise_construct,
            std::forward_as_tuple(x.first),
            std::forward_as_tuple(heap_range));
    }

    return std::make_shared<DXBindingSet>(*this, m_is_compute, descriptor_ranges, m_binding_layout);
}

const std::vector<std::shared_ptr<DXShader>>& DXProgram::GetShaders() const
{
    return m_shaders;
}

const ComPtr<ID3D12RootSignature>& DXProgram::GetRootSignature() const
{
    return m_root_signature;
}
