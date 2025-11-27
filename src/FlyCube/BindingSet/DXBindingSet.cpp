#include "BindingSet/DXBindingSet.h"

#include "BindingSetLayout/DXBindingSetLayout.h"
#include "Device/DXDevice.h"
#include "GPUDescriptorPool/DXGPUDescriptorPoolRange.h"
#include "View/DXView.h"

namespace {

void SetRootDescriptorTable(const ComPtr<ID3D12GraphicsCommandList>& command_list,
                            bool is_compute,
                            uint32_t root_param_index,
                            D3D12_GPU_DESCRIPTOR_HANDLE base_descriptor)
{
    if (is_compute) {
        command_list->SetComputeRootDescriptorTable(root_param_index, base_descriptor);
    } else {
        command_list->SetGraphicsRootDescriptorTable(root_param_index, base_descriptor);
    }
}

void SeRoot32BitConstants(const ComPtr<ID3D12GraphicsCommandList>& command_list,
                          bool is_compute,
                          uint32_t root_param_index,
                          const std::span<uint32_t>& data)
{
    if (is_compute) {
        command_list->SetComputeRoot32BitConstants(root_param_index, data.size(), data.data(), 0);
    } else {
        command_list->SetGraphicsRoot32BitConstants(root_param_index, data.size(), data.data(), 0);
    }
}

} // namespace

DXBindingSet::DXBindingSet(DXDevice& device, const std::shared_ptr<DXBindingSetLayout>& layout)
    : device_(device)
    , layout_(layout)
{
    for (const auto& [descriptor_type, count] : layout_->GetHeapDescs()) {
        auto heap_range =
            std::make_shared<DXGPUDescriptorPoolRange>(device_.GetGPUDescriptorPool().Allocate(descriptor_type, count));
        descriptor_ranges_.emplace(descriptor_type, heap_range);
    }

    size_t total_constants = 0;
    for (const auto& [bind_key, desc] : layout_->GetConstantsLayout()) {
        constants_data_offsets_[bind_key] = total_constants;
        total_constants += desc.num_constants;
    }
    constants_data_.resize(total_constants);

    CreateConstantsFallbackBuffer(device_, layout->GetFallbackConstants());
    for (const auto& [bind_key, view] : fallback_constants_buffer_views_) {
        WriteDescriptor({ bind_key, view });
    }
}

void DXBindingSet::WriteBindings(const std::vector<BindingDesc>& bindings)
{
    WriteBindingsAndConstants(bindings, {});
}

void DXBindingSet::WriteBindingsAndConstants(const std::vector<BindingDesc>& bindings,
                                             const std::vector<BindingConstantsData>& constants)
{
    for (const auto& binding : bindings) {
        WriteDescriptor(binding);
    }

    for (const auto& [bind_key, data] : constants) {
        if (layout_->GetConstantsLayout().contains(bind_key)) {
            size_t offset = constants_data_offsets_.at(bind_key);
            memcpy(&constants_data_[offset], data.data(), data.size());
        } else {
            fallback_constants_buffer_->UpdateUploadBuffer(fallback_constants_buffer_offsets_.at(bind_key), data.data(),
                                                           data.size());
        }
    }
}

std::vector<ComPtr<ID3D12DescriptorHeap>> DXBindingSet::Apply(const ComPtr<ID3D12GraphicsCommandList>& command_list)
{
    std::map<D3D12_DESCRIPTOR_HEAP_TYPE, ComPtr<ID3D12DescriptorHeap>> heap_map;
    for (const auto& [_, table] : layout_->GetDescriptorTables()) {
        D3D12_DESCRIPTOR_HEAP_TYPE heap_type = table.heap_type;
        ComPtr<ID3D12DescriptorHeap> heap;
        if (table.bindless) {
            heap = device_.GetGPUDescriptorPool().GetHeap(heap_type);
        } else {
            heap = descriptor_ranges_.at(heap_type)->GetHeap();
        }

        auto it = heap_map.find(heap_type);
        if (it == heap_map.end()) {
            heap_map.emplace(heap_type, device_.GetGPUDescriptorPool().GetHeap(heap_type));
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

    for (const auto& [root_param_index, table] : layout_->GetDescriptorTables()) {
        D3D12_DESCRIPTOR_HEAP_TYPE heap_type = table.heap_type;
        D3D12_GPU_DESCRIPTOR_HANDLE base_descriptor = {};
        if (table.bindless) {
            base_descriptor = device_.GetGPUDescriptorPool().GetHeap(heap_type)->GetGPUDescriptorHandleForHeapStart();
        } else {
            auto& heap_range = descriptor_ranges_.at(heap_type);
            base_descriptor = heap_range->GetGpuHandle(table.heap_offset);
        }
        SetRootDescriptorTable(command_list, table.is_compute, root_param_index, base_descriptor);
    }

    for (const auto& [bind_key, desc] : layout_->GetConstantsLayout()) {
        size_t offset = constants_data_offsets_.at(bind_key);
        std::span<uint32_t> data(&constants_data_[offset], desc.num_constants);
        SeRoot32BitConstants(command_list, desc.is_compute, desc.root_param_index, data);
    }

    return descriptor_heaps;
}

void DXBindingSet::WriteDescriptor(const BindingDesc& binding)
{
    decltype(auto) binding_layout = layout_->GetLayout().at(binding.bind_key);
    std::shared_ptr<DXGPUDescriptorPoolRange> heap_range = descriptor_ranges_.at(binding_layout.heap_type);
    decltype(auto) src_cpu_handle = binding.view->As<DXView>().GetHandle();
    heap_range->CopyCpuHandle(binding_layout.heap_offset, src_cpu_handle);
}
