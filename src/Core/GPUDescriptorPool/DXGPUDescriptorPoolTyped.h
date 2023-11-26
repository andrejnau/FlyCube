#pragma once
#include "GPUDescriptorPool/DXGPUDescriptorPoolRange.h"
#include "Instance/BaseTypes.h"
#include "Utilities/DXUtility.h"

#include <directx/d3d12.h>
#include <wrl.h>

#include <algorithm>
#include <map>
#include <memory>
using namespace Microsoft::WRL;

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
    DXDevice& m_device;
    D3D12_DESCRIPTOR_HEAP_TYPE m_type;
    size_t m_offset;
    size_t m_size;
    ComPtr<ID3D12DescriptorHeap> m_heap;
    D3D12_CPU_DESCRIPTOR_HANDLE m_cpu_handle;
    D3D12_GPU_DESCRIPTOR_HANDLE m_gpu_handle;
    ComPtr<ID3D12DescriptorHeap> m_heap_readable;
    D3D12_CPU_DESCRIPTOR_HANDLE m_cpu_handle_readable;
    std::multimap<size_t, size_t> m_empty_ranges;
};
