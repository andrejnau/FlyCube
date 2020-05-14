#include "BindingSet/DXBindingSet.h"
#include <GPUDescriptorPool/DXGPUDescriptorPoolRange.h>

DXBindingSet::DXBindingSet(DXProgram& program,
                           std::map<D3D12_DESCRIPTOR_HEAP_TYPE, std::pair<bool, std::reference_wrapper<DXGPUDescriptorPoolRange>>> descriptor_ranges,
                           std::map<std::tuple<ShaderType, D3D12_DESCRIPTOR_RANGE_TYPE>, BindingLayout> binding_layout,
                           std::vector<ID3D12DescriptorHeap*> descriptor_heaps)
    : m_descriptor_ranges(descriptor_ranges)
    , m_binding_layout(binding_layout)
    , m_descriptor_heaps(descriptor_heaps)
{
}

void SetRootDescriptorTable(const ComPtr<ID3D12GraphicsCommandList>& command_list, uint32_t RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor)
{
    if (RootParameterIndex == -1)
        return;
    command_list->SetGraphicsRootDescriptorTable(RootParameterIndex, BaseDescriptor);
}


void DXBindingSet::Apply(const ComPtr<ID3D12GraphicsCommandList>& command_list)
{
    if (m_descriptor_heaps.size())
        command_list->SetDescriptorHeaps(m_descriptor_heaps.size(), m_descriptor_heaps.data());

    for (auto& x : m_binding_layout)
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
            if (!m_descriptor_ranges.count(heap_type))
                continue;
            std::reference_wrapper<DXGPUDescriptorPoolRange> heap_range = m_descriptor_ranges.find(heap_type)->second.second;
            if (x.second.root_param_index != -1)
                SetRootDescriptorTable(command_list, x.second.root_param_index, heap_range.get().GetGpuHandle(x.second.table.root_param_heap_offset));
        }
    }
}
