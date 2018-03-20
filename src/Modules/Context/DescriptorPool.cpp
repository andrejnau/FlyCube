#include "Context/DescriptorPool.h"

#include <assert.h>

#include "Context/DX12Context.h"

DescriptorHeapRange::DescriptorHeapRange(ComPtr<ID3D12DescriptorHeap>& heap, size_t offset, size_t size, size_t increment_size, D3D12_DESCRIPTOR_HEAP_TYPE type)
    : m_heap(heap)
    , m_offset(offset)
    , m_size(size)
    , m_increment_size(increment_size)
    , m_type(type)
{
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapRange::GetCpuHandle(size_t offset) const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        m_heap->GetCPUDescriptorHandleForHeapStart(),
        m_offset + offset,
        m_increment_size);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapRange::GetGpuHandle(size_t offset) const
{
    return CD3DX12_GPU_DESCRIPTOR_HANDLE(
        m_heap->GetGPUDescriptorHandleForHeapStart(),
        m_offset + offset,
        m_increment_size);
}

DescriptorHeapAllocator::DescriptorHeapAllocator(DX12Context& context, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
    : m_context(context)
    , m_type(type)
    , m_flags(flags)
    , m_offset(0)
    , m_size(0)
{
}

DescriptorHeapRange DescriptorHeapAllocator::Allocate(size_t count)
{
    if (m_offset + count > m_size)
    {
        assert(!(m_size > 0 && m_flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE));
        ResizeHeap(std::max(m_offset + count, 2 * (m_size + 1)));
    }
    m_offset += count;
    return DescriptorHeapRange(m_heap, m_offset - count, count, m_context.device->GetDescriptorHandleIncrementSize(m_type), m_type);
}

void DescriptorHeapAllocator::ResizeHeap(size_t req_size)
{
    if (m_size >= req_size)
        return;

    ComPtr<ID3D12DescriptorHeap> heap;
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.NumDescriptors = req_size;
    heap_desc.Flags = m_flags;
    heap_desc.Type = m_type;
    ASSERT_SUCCEEDED(m_context.device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap)));
    if (m_size > 0 && m_flags != D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
    {
        m_context.device->CopyDescriptorsSimple(
            m_size,
            heap->GetCPUDescriptorHandleForHeapStart(),
            m_heap->GetCPUDescriptorHandleForHeapStart(),
            m_type);
    }
    m_size = heap_desc.NumDescriptors;
    m_heap = heap;
}

void DescriptorHeapAllocator::ResetHeap()
{
    m_offset = 0;
}

DescriptorPoolByType::DescriptorPoolByType(DX12Context& context, D3D12_DESCRIPTOR_HEAP_TYPE type)
    : m_context(context)
    , m_heap_alloc(context, type, D3D12_DESCRIPTOR_HEAP_FLAG_NONE)
{
}

DescriptorByResource DescriptorPoolByType::GetDescriptor(size_t bind_id, DX12Resource::Ptr& res)
{
    bool exist = true;
    auto it = res->descriptors.find(bind_id);
    if (it == res->descriptors.end())
    {
        exist = false;
        it = res->descriptors.emplace(std::piecewise_construct,
            std::forward_as_tuple(bind_id),
            std::forward_as_tuple(m_heap_alloc.Allocate(1))).first;
    }
    return { it->second, exist };
}

DescriptorHeapRange DescriptorPoolByType::AllocateDescriptor()
{
    return m_heap_alloc.Allocate(1);
}

DescriptorPool::DescriptorPool(DX12Context& context)
    : m_context(context)
    , m_resource(m_context, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    , m_sampler(m_context, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    , m_rtv(m_context, D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
    , m_dsv(m_context, D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
    , m_shader_resource(m_context, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
    , m_shader_sampler(m_context, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
{
}

DescriptorByResource DescriptorPool::GetDescriptor(ResourceType res_type, size_t bind_id, DX12Resource::Ptr& res)
{
    DescriptorPoolByType& pool = SelectHeap(res_type);
    return pool.GetDescriptor(bind_id, res);
}

DescriptorHeapRange DescriptorPool::AllocateDescriptor(ResourceType res_type)
{
    DescriptorPoolByType& pool = SelectHeap(res_type);
    return pool.AllocateDescriptor();
}

void DescriptorPool::OnFrameBegin()
{
    m_shader_resource.ResetHeap();
    m_shader_sampler.ResetHeap();
    m_need_resources = 0;
    m_need_samplers = 0;
    first_frame_alloc = true;
}

void DescriptorPool::ReqFrameDescription(ResourceType res_type, size_t count)
{
    switch (res_type)
    {
    case ResourceType::kSampler:
        m_need_samplers += count;
        break;
    default:
        m_need_resources += count;
    }
}

DescriptorHeapRange DescriptorPool::Allocate(ResourceType res_type, size_t count)
{
    if (first_frame_alloc)
    {
        m_shader_sampler.ResizeHeap(m_need_samplers);
        m_shader_resource.ResizeHeap(m_need_resources);
        first_frame_alloc = false;
    }

    switch (res_type)
    {
    case ResourceType::kSampler:
        return m_shader_sampler.Allocate(count);
    default:
        return m_shader_resource.Allocate(count);
    }
}

DescriptorPoolByType& DescriptorPool::SelectHeap(ResourceType res_type)
{
    switch (res_type)
    {
    case ResourceType::kSrv:
    case ResourceType::kUav:
    case ResourceType::kCbv:
        return m_resource;
    case ResourceType::kSampler:
        return m_sampler;
    case ResourceType::kRtv:
        return m_rtv;
    case ResourceType::kDsv:
        return m_dsv;
    default:
    {
        throw "fatal failure";
    }
    }
}
