#pragma once
#include "CPUDescriptorPool/DXCPUDescriptorHandle.h"
#include "Instance/BaseTypes.h"
#include "Utilities/DXUtility.h"

#if defined(_WIN32)
#include <wrl.h>
#else
#include <wsl/wrladapter.h>
#endif

#include <directx/d3d12.h>

#include <algorithm>
#include <map>
#include <memory>

using Microsoft::WRL::ComPtr;

class DXDevice;

class DXCPUDescriptorPoolTyped {
public:
    DXCPUDescriptorPoolTyped(DXDevice& device, D3D12_DESCRIPTOR_HEAP_TYPE type);
    std::shared_ptr<DXCPUDescriptorHandle> Allocate(size_t count);
    void ResizeHeap(size_t req_size);

private:
    DXDevice& device_;
    D3D12_DESCRIPTOR_HEAP_TYPE type_;
    size_t offset_;
    size_t size_;
    ComPtr<ID3D12DescriptorHeap> heap_;
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle_;
};
