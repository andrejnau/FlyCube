#include "GPUDescriptorPool/DXGPUDescriptorPoolTyped.h"

#include "Device/DXDevice.h"

#include <directx/d3dx12.h>

DXGPUDescriptorPoolTyped::DXGPUDescriptorPoolTyped(DXDevice& device, D3D12_DESCRIPTOR_HEAP_TYPE type)
    : device_(device)
    , type_(type)
    , offset_(0)
    , size_(0)
{
}

DXGPUDescriptorPoolRange DXGPUDescriptorPoolTyped::Allocate(size_t count)
{
    auto it = empty_ranges_.lower_bound(count);
    if (it != empty_ranges_.end()) {
        size_t offset = it->second;
        size_t size = it->first;
        empty_ranges_.erase(it);
        return DXGPUDescriptorPoolRange(*this, device_, heap_, cpu_handle_, gpu_handle_, heap_readable_,
                                        cpu_handle_readable_, offset, size,
                                        device_.GetDevice()->GetDescriptorHandleIncrementSize(type_), type_);
    }
    if (offset_ + count > size_) {
        ResizeHeap(std::max(offset_ + count, 2 * (size_ + 1)));
    }
    offset_ += count;
    return DXGPUDescriptorPoolRange(*this, device_, heap_, cpu_handle_, gpu_handle_, heap_readable_,
                                    cpu_handle_readable_, offset_ - count, count,
                                    device_.GetDevice()->GetDescriptorHandleIncrementSize(type_), type_);
}

void DXGPUDescriptorPoolTyped::ResizeHeap(size_t req_size)
{
    if (size_ >= req_size) {
        return;
    }

    ComPtr<ID3D12DescriptorHeap> heap;
    ComPtr<ID3D12DescriptorHeap> heap_readable;
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.NumDescriptors = static_cast<uint32_t>(req_size);
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heap_desc.Type = type_;
    CHECK_HRESULT(device_.GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap)));

    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    CHECK_HRESULT(device_.GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap_readable)));

    if (size_ > 0) {
        device_.GetDevice()->CopyDescriptorsSimple(size_, heap_readable->GetCPUDescriptorHandleForHeapStart(),
                                                   cpu_handle_readable_, type_);
        device_.GetDevice()->CopyDescriptorsSimple(size_, heap->GetCPUDescriptorHandleForHeapStart(),
                                                   cpu_handle_readable_, type_);
    }

    size_ = heap_desc.NumDescriptors;
    heap_ = heap;
    heap_readable_ = heap_readable;
    cpu_handle_ = heap_->GetCPUDescriptorHandleForHeapStart();
    gpu_handle_ = heap_->GetGPUDescriptorHandleForHeapStart();
    cpu_handle_readable_ = heap_readable_->GetCPUDescriptorHandleForHeapStart();
}

void DXGPUDescriptorPoolTyped::OnRangeDestroy(size_t offset, size_t size)
{
    empty_ranges_.emplace(size, offset);
}

void DXGPUDescriptorPoolTyped::ResetHeap()
{
    offset_ = 0;
}

ComPtr<ID3D12DescriptorHeap> DXGPUDescriptorPoolTyped::GetHeap()
{
    return heap_;
}
