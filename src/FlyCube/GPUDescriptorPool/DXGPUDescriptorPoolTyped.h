#pragma once
#include "GPUDescriptorPool/DXGPUDescriptorPoolRange.h"
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

class DXGPUDescriptorPoolTyped {
public:
    DXGPUDescriptorPoolTyped(DXDevice& device, D3D12_DESCRIPTOR_HEAP_TYPE type);
    DXGPUDescriptorPoolRange Allocate(size_t count);
    void ResizeHeap(size_t req_size);
    void OnRangeDestroy(size_t offset, size_t size);
    void ResetHeap();
    ComPtr<ID3D12DescriptorHeap> GetHeap();

private:
    DXDevice& device_;
    D3D12_DESCRIPTOR_HEAP_TYPE type_;
    size_t offset_;
    size_t size_;
    ComPtr<ID3D12DescriptorHeap> heap_;
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle_;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle_;
    ComPtr<ID3D12DescriptorHeap> heap_readable_;
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle_readable_;
    std::multimap<size_t, size_t> empty_ranges_;
};
