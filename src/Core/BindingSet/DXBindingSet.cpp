#include "BindingSet/DXBindingSet.h"
#include <GPUDescriptorPool/DXGPUDescriptorPoolRange.h>
#include <Program/DXProgram.h>
#include <Device/DXDevice.h>

DXBindingSet::DXBindingSet(DXProgram& program, bool is_compute,
                           std::map<D3D12_DESCRIPTOR_HEAP_TYPE, std::shared_ptr<DXGPUDescriptorPoolRange>> descriptor_ranges,
                           std::map<std::tuple<ShaderType, D3D12_DESCRIPTOR_RANGE_TYPE, uint32_t /*space*/, bool /*bindless*/>, BindingLayout> binding_layout)
    : m_device(program.GetDevice())
    , m_is_compute(is_compute)
    , m_descriptor_ranges(descriptor_ranges)
    , m_binding_layout(binding_layout)
{
}

void SetRootDescriptorTable(bool is_compute, const ComPtr<ID3D12GraphicsCommandList>& command_list, uint32_t RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor)
{
    if (RootParameterIndex == -1)
        return;
    if (is_compute)
        command_list->SetComputeRootDescriptorTable(RootParameterIndex, BaseDescriptor);
    else
        command_list->SetGraphicsRootDescriptorTable(RootParameterIndex, BaseDescriptor);
}

std::vector<ComPtr<ID3D12DescriptorHeap>> DXBindingSet::Apply(const ComPtr<ID3D12GraphicsCommandList>& command_list)
{
    std::vector<ComPtr<ID3D12DescriptorHeap>> descriptor_heaps;
    std::vector<ID3D12DescriptorHeap*> descriptor_heaps_ptr;
    for (decltype(auto) descriptor_range : m_descriptor_ranges)
    {
        if (descriptor_range.second->GetSize() > 0)
        {
            descriptor_heaps.emplace_back(descriptor_range.second->GetHeap());
            descriptor_heaps_ptr.emplace_back(descriptor_heaps.back().Get());
        }
    }
    if (descriptor_heaps_ptr.size())
        command_list->SetDescriptorHeaps(descriptor_heaps_ptr.size(), descriptor_heaps_ptr.data());

    for (auto& x : m_binding_layout)
    {
        if (x.second.root_param_index == -1)
            continue;
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
            if (std::get<3>(x.first))
            {
                SetRootDescriptorTable(m_is_compute, command_list, x.second.root_param_index, m_device.GetGPUDescriptorPool().GetHeap(heap_type)->GetGPUDescriptorHandleForHeapStart());
                continue;
            }

            if (!m_descriptor_ranges.count(heap_type))
                continue;
            std::shared_ptr<DXGPUDescriptorPoolRange> heap_range = m_descriptor_ranges.at(heap_type);
            SetRootDescriptorTable(m_is_compute, command_list, x.second.root_param_index, heap_range->GetGpuHandle(x.second.table.root_param_heap_offset));
        }
    }
    return descriptor_heaps;
}
