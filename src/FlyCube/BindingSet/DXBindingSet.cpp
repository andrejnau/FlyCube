#include "BindingSet/DXBindingSet.h"

#include "BindingSetLayout/DXBindingSetLayout.h"
#include "Device/DXDevice.h"
#include "GPUDescriptorPool/DXGPUDescriptorPoolRange.h"
#include "Program/ProgramBase.h"
#include "View/DXView.h"

DXBindingSet::DXBindingSet(DXDevice& device, const std::shared_ptr<DXBindingSetLayout>& layout)
    : m_device(device)
    , m_layout(layout)
{
    for (const auto& desc : m_layout->GetHeapDescs()) {
        std::shared_ptr<DXGPUDescriptorPoolRange> heap_range = std::make_shared<DXGPUDescriptorPoolRange>(
            m_device.GetGPUDescriptorPool().Allocate(desc.first, desc.second));
        m_descriptor_ranges.emplace(std::piecewise_construct, std::forward_as_tuple(desc.first),
                                    std::forward_as_tuple(heap_range));
    }
}

void DXBindingSet::WriteBindings(const std::vector<BindingDesc>& bindings)
{
    for (const auto& binding : bindings) {
        if (!binding.view) {
            continue;
        }
        decltype(auto) binding_layout = m_layout->GetLayout().at(binding.bind_key);
        std::shared_ptr<DXGPUDescriptorPoolRange> heap_range = m_descriptor_ranges.at(binding_layout.heap_type);
        decltype(auto) src_cpu_handle = binding.view->As<DXView>().GetHandle();
        heap_range->CopyCpuHandle(binding_layout.heap_offset, src_cpu_handle);
    }
}

void SetRootDescriptorTable(const ComPtr<ID3D12GraphicsCommandList>& command_list,
                            uint32_t root_parameter,
                            bool is_compute,
                            D3D12_GPU_DESCRIPTOR_HANDLE base_descriptor)
{
    if (is_compute) {
        command_list->SetComputeRootDescriptorTable(root_parameter, base_descriptor);
    } else {
        command_list->SetGraphicsRootDescriptorTable(root_parameter, base_descriptor);
    }
}

std::vector<ComPtr<ID3D12DescriptorHeap>> DXBindingSet::Apply(const ComPtr<ID3D12GraphicsCommandList>& command_list)
{
    std::map<D3D12_DESCRIPTOR_HEAP_TYPE, ComPtr<ID3D12DescriptorHeap>> heap_map;
    for (const auto& table : m_layout->GetDescriptorTables()) {
        D3D12_DESCRIPTOR_HEAP_TYPE heap_type = table.second.heap_type;
        ComPtr<ID3D12DescriptorHeap> heap;
        if (table.second.bindless) {
            heap = m_device.GetGPUDescriptorPool().GetHeap(heap_type);
        } else {
            heap = m_descriptor_ranges.at(heap_type)->GetHeap();
        }

        auto it = heap_map.find(heap_type);
        if (it == heap_map.end()) {
            heap_map.emplace(heap_type, m_device.GetGPUDescriptorPool().GetHeap(heap_type));
        } else {
            assert(it->second == heap);
        }
    }

    std::vector<ComPtr<ID3D12DescriptorHeap>> descriptor_heaps;
    std::vector<ID3D12DescriptorHeap*> descriptor_heaps_ptr;
    for (const auto& heap : heap_map) {
        descriptor_heaps.emplace_back(heap.second);
        descriptor_heaps_ptr.emplace_back(heap.second.Get());
    }
    if (descriptor_heaps_ptr.size()) {
        command_list->SetDescriptorHeaps(descriptor_heaps_ptr.size(), descriptor_heaps_ptr.data());
    }

    for (const auto& table : m_layout->GetDescriptorTables()) {
        D3D12_DESCRIPTOR_HEAP_TYPE heap_type = table.second.heap_type;
        D3D12_GPU_DESCRIPTOR_HANDLE base_descriptor = {};
        if (table.second.bindless) {
            base_descriptor = m_device.GetGPUDescriptorPool().GetHeap(heap_type)->GetGPUDescriptorHandleForHeapStart();
        } else {
            decltype(auto) heap_range = m_descriptor_ranges.at(heap_type);
            base_descriptor = heap_range->GetGpuHandle(table.second.heap_offset);
        }
        SetRootDescriptorTable(command_list, table.first, table.second.is_compute, base_descriptor);
    }
    return descriptor_heaps;
}
