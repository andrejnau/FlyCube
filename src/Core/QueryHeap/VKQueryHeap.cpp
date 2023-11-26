#include "QueryHeap/VKQueryHeap.h"

#include "Device/VKDevice.h"

VKQueryHeap::VKQueryHeap(VKDevice& device, QueryHeapType type, uint32_t count)
    : m_device(device)
{
    assert(type == QueryHeapType::kAccelerationStructureCompactedSize);
    m_query_type = vk::QueryType::eAccelerationStructureCompactedSizeKHR;
    vk::QueryPoolCreateInfo desc = {};
    desc.queryCount = count;
    desc.queryType = m_query_type;
    m_query_pool = m_device.GetDevice().createQueryPoolUnique(desc);
}

QueryHeapType VKQueryHeap::GetType() const
{
    return QueryHeapType::kAccelerationStructureCompactedSize;
}

vk::QueryType VKQueryHeap::GetQueryType() const
{
    return m_query_type;
}

vk::QueryPool VKQueryHeap::GetQueryPool() const
{
    return m_query_pool.get();
}
