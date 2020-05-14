#include "Program/DXProgram.h"
#include <Device/DXDevice.h>
#include <Shader/DXReflector.h>
#include <Shader/DXShader.h>
#include <d3dx12.h>

DXProgram::DXProgram(DXDevice& device, const std::vector<std::shared_ptr<Shader>>& shaders)
    : m_device(device)
{
    for (auto& shader : shaders)
    {
        m_shaders.emplace_back(std::static_pointer_cast<DXShader>(shader));
    }

    size_t num_resources = 0;
    size_t num_samplers = 0;

    std::vector<D3D12_ROOT_PARAMETER> root_parameters;
    std::deque<std::array<D3D12_DESCRIPTOR_RANGE, 4>> descriptor_table_ranges;
    for (auto& shader : m_shaders)
    {
        ShaderType shader_type = shader->GetType();
        ComPtr<ID3DBlob> shader_blob = shader->GetBlob();
        uint32_t num_cbv = 0;
        uint32_t num_srv = 0;
        uint32_t num_uav = 0;
        uint32_t num_sampler = 0;

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

            if (shader_type == ShaderType::kPixel)
            {
                for (uint32_t i = 0; i < desc.OutputParameters; ++i)
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
            DXReflect(shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(), IID_PPV_ARGS(&library_reflector));
            if (library_reflector)
            {
                D3D12_LIBRARY_DESC lib_desc = {};
                library_reflector->GetDesc(&lib_desc);
                for (int j = 0; j < lib_desc.FunctionCount; ++j)
                {
                    auto reflector = library_reflector->GetFunctionByIndex(j);
                    D3D12_FUNCTION_DESC desc = {};
                    reflector->GetDesc(&desc);
                    for (uint32_t i = 0; i < desc.BoundResources; ++i)
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

        m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_SRV}].type = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_SRV}].table.root_param_heap_offset = num_resources;
        m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_SRV}].table.heap_offset = num_resources;

        m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_UAV}].type = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_UAV}].table.root_param_heap_offset = num_resources;
        m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_UAV}].table.heap_offset = num_resources + num_srv;

        if (m_use_cbv_table)
        {
            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_CBV}].type = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_CBV}].table.root_param_heap_offset = num_resources;
            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_CBV}].table.heap_offset = num_resources + num_srv + num_uav;
        }
        else
        {
            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_CBV}].type = D3D12_ROOT_PARAMETER_TYPE_CBV;
            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_CBV}].view.root_param_num = num_cbv;
        }

        m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER}].type = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER}].table.root_param_heap_offset = num_samplers;
        m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER}].table.heap_offset = num_samplers;

        if (!num_srv)
            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_SRV}].root_param_index = -1;
        if (!num_uav)
            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_UAV}].root_param_index = -1;
        if (!num_cbv)
            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_CBV}].root_param_index = -1;
        if (!num_sampler)
            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER}].root_param_index = -1;

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

            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_SRV}].root_param_index = static_cast<uint32_t>(root_parameters.size());
            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_UAV}].root_param_index = static_cast<uint32_t>(root_parameters.size());
            if (m_use_cbv_table)
                m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_CBV}].root_param_index = static_cast<uint32_t>(root_parameters.size());

            root_parameters.emplace_back();
            root_parameters.back().ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            root_parameters.back().DescriptorTable = descriptorTableTexture;
            root_parameters.back().ShaderVisibility = visibility;
        }

        if (!m_use_cbv_table && num_cbv > 0)
        {
            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_CBV}].root_param_index = static_cast<uint32_t>(root_parameters.size());

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

            m_binding_layout[{shader_type, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER}].root_param_index = static_cast<uint32_t>(root_parameters.size());

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
    root_signature_desc.Init(static_cast<uint32_t>(root_parameters.size()),
        root_parameters.data(),
        0,
        nullptr,
        root_signature_flags);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error_blob;
    ASSERT_SUCCEEDED(D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error_blob),
        "%s", (char*)error_blob->GetBufferPointer());
    ASSERT_SUCCEEDED(device.GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_root_signature)));
}

const std::vector<std::shared_ptr<DXShader>>& DXProgram::GetShaders() const
{
    return m_shaders;
}

const ComPtr<ID3D12RootSignature>& DXProgram::GetRootSignature() const
{
    return m_root_signature;
}