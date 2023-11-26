#pragma once
#include "Instance/BaseTypes.h"
#include "Utilities/DXUtility.h"

#include <directx/d3d12.h>
#include <wrl.h>

#include <algorithm>
#include <map>
#include <memory>
using namespace Microsoft::WRL;

class DXDevice;

class DXCPUDescriptorHandle {
public:
    DXCPUDescriptorHandle(DXDevice& device,
                          ComPtr<ID3D12DescriptorHeap>& heap,
                          D3D12_CPU_DESCRIPTOR_HANDLE& cpu_handle,
                          size_t offset,
                          size_t size,
                          uint32_t increment_size,
                          D3D12_DESCRIPTOR_HEAP_TYPE type);
    D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(size_t offset = 0) const;

private:
    std::reference_wrapper<DXDevice> m_device;
    std::reference_wrapper<ComPtr<ID3D12DescriptorHeap>> m_heap;
    std::reference_wrapper<D3D12_CPU_DESCRIPTOR_HANDLE> m_cpu_handle;
    size_t m_offset;
    size_t m_size;
    uint32_t m_increment_size;
    D3D12_DESCRIPTOR_HEAP_TYPE m_type;
};
