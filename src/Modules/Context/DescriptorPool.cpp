#include "Context/DescriptorPool.h"

#include <assert.h>

#include "Context/DX12Context.h"

DescriptorHeapRange::DescriptorHeapRange(
    DX12Context& context,
    ComPtr<ID3D12DescriptorHeap>& heap,
    D3D12_CPU_DESCRIPTOR_HANDLE& cpu_handle,
    D3D12_GPU_DESCRIPTOR_HANDLE& gpu_handle,
    ComPtr<ID3D12DescriptorHeap>& heap_readable,
    D3D12_CPU_DESCRIPTOR_HANDLE& cpu_handle_readable,
    size_t offset,
    size_t size,
    uint32_t increment_size,
    D3D12_DESCRIPTOR_HEAP_TYPE type)
    : m_context(context)
    , m_heap(heap)
    , m_cpu_handle(cpu_handle)
    , m_gpu_handle(gpu_handle)
    , m_heap_readable(heap_readable)
    , m_cpu_handle_readable(cpu_handle_readable)
    , m_offset(offset)
    , m_size(size)
    , m_increment_size(increment_size)
    , m_type(type)
{
}

void DescriptorHeapRange::CopyCpuHandle(size_t dst_offset, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    D3D12_CPU_DESCRIPTOR_HANDLE self = GetCpuHandle(m_cpu_handle, dst_offset);
    m_context.get().device->CopyDescriptors(
        1, &self, nullptr,
        1, &handle, nullptr,
        m_type);
    self = GetCpuHandle(m_cpu_handle_readable, dst_offset);
    m_context.get().device->CopyDescriptors(
        1, &self, nullptr,
        1, &handle, nullptr,
        m_type);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapRange::GetCpuHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle, size_t offset) const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        handle,
        static_cast<INT>(m_offset + offset),
        m_increment_size);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapRange::GetGpuHandle(size_t offset) const
{
    return CD3DX12_GPU_DESCRIPTOR_HANDLE(
        m_gpu_handle,
        static_cast<INT>(m_offset + offset),
        m_increment_size);
}

const ComPtr<ID3D12DescriptorHeap>& DescriptorHeapRange::GetHeap() const
{
    return m_heap;
}

size_t DescriptorHeapRange::GetSize() const
{
    return m_size;
}

DescriptorHeapAllocator::DescriptorHeapAllocator(DX12Context& context, D3D12_DESCRIPTOR_HEAP_TYPE type)
    : m_context(context)
    , m_type(type)
    , m_offset(0)
    , m_size(0)
{
}

DescriptorHeapRange DescriptorHeapAllocator::Allocate(size_t count)
{
    if (m_offset + count > m_size)
        ResizeHeap(std::max(m_offset + count, 2 * (m_size + 1)));
    m_offset += count;
    return DescriptorHeapRange(m_context, m_heap, m_cpu_handle, m_gpu_handle, m_heap_readable, m_cpu_handle_readable, m_offset - count, count, m_context.device->GetDescriptorHandleIncrementSize(m_type), m_type);
}

void DescriptorHeapAllocator::ResizeHeap(size_t req_size)
{
    if (m_size >= req_size)
        return;

    ComPtr<ID3D12DescriptorHeap> heap;
    ComPtr<ID3D12DescriptorHeap> heap_readable;
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.NumDescriptors = static_cast<uint32_t>(req_size);
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heap_desc.Type = m_type;
    ASSERT_SUCCEEDED(m_context.device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap)));

    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ASSERT_SUCCEEDED(m_context.device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap_readable)));

    if (m_size > 0)
    {
        m_context.device->CopyDescriptorsSimple(m_size, heap_readable->GetCPUDescriptorHandleForHeapStart(), m_cpu_handle_readable, m_type);
        m_context.device->CopyDescriptorsSimple(m_size, heap->GetCPUDescriptorHandleForHeapStart(), m_cpu_handle_readable, m_type);
    }

    m_context.QueryOnDelete(m_heap);
    
    m_size = heap_desc.NumDescriptors;
    m_heap = heap;
    m_heap_readable = heap_readable;
    m_cpu_handle = m_heap->GetCPUDescriptorHandleForHeapStart();
    m_gpu_handle = m_heap->GetGPUDescriptorHandleForHeapStart();
    m_cpu_handle_readable = m_heap_readable->GetCPUDescriptorHandleForHeapStart();
}

void DescriptorHeapAllocator::ResetHeap()
{
    m_offset = 0;
}

DescriptorPool::DescriptorPool(DX12Context& context)
    : m_context(context)
    , m_shader_resource(m_context, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    , m_shader_sampler(m_context, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
{
}

DescriptorHeapRange DescriptorPool::Allocate(D3D12_DESCRIPTOR_HEAP_TYPE descriptor_type, size_t count)
{
    switch (descriptor_type)
    {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        return m_shader_resource.Allocate(count);
    case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
        return m_shader_sampler.Allocate(count);
    default:
        throw std::runtime_error("wrong descriptor type");
    }
}
