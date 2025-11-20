#include "CPUDescriptorPool/DXCPUDescriptorPoolTyped.h"

#include "Device/DXDevice.h"

#include <directx/d3dx12.h>

DXCPUDescriptorPoolTyped::DXCPUDescriptorPoolTyped(DXDevice& device, D3D12_DESCRIPTOR_HEAP_TYPE type)
    : device_(device)
    , type_(type)
    , offset_(0)
    , size_(0)
{
}

std::shared_ptr<DXCPUDescriptorHandle> DXCPUDescriptorPoolTyped::Allocate(size_t count)
{
    if (offset_ + count > size_) {
        ResizeHeap(std::max(offset_ + count, 2 * (size_ + 1)));
    }
    offset_ += count;
    return std::make_shared<DXCPUDescriptorHandle>(device_, heap_, cpu_handle_, offset_ - count, count,
                                                   device_.GetDevice()->GetDescriptorHandleIncrementSize(type_), type_);
}

void DXCPUDescriptorPoolTyped::ResizeHeap(size_t req_size)
{
    if (size_ >= req_size) {
        return;
    }

    ComPtr<ID3D12DescriptorHeap> heap;
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.NumDescriptors = static_cast<uint32_t>(req_size);
    heap_desc.Type = type_;
    CHECK_HRESULT(device_.GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap)));
    if (size_ > 0) {
        device_.GetDevice()->CopyDescriptorsSimple(static_cast<uint32_t>(size_),
                                                   heap->GetCPUDescriptorHandleForHeapStart(),
                                                   heap_->GetCPUDescriptorHandleForHeapStart(), type_);
    }

    size_ = heap_desc.NumDescriptors;
    heap_ = heap;
    cpu_handle_ = heap_->GetCPUDescriptorHandleForHeapStart();
}
