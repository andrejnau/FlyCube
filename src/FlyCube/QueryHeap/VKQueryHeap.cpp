#include "QueryHeap/VKQueryHeap.h"

#include "Device/VKDevice.h"

VKQueryHeap::VKQueryHeap(VKDevice& device, QueryHeapType type, uint32_t count)
    : device_(device)
{
    query_type_ = vk::QueryType::eAccelerationStructureCompactedSizeKHR;
    vk::QueryPoolCreateInfo desc = {};
    desc.queryCount = count;
    desc.queryType = query_type_;
    query_pool_ = device_.GetDevice().createQueryPoolUnique(desc);
}

QueryHeapType VKQueryHeap::GetType() const
{
    return QueryHeapType::kAccelerationStructureCompactedSize;
}

vk::QueryType VKQueryHeap::GetQueryType() const
{
    return query_type_;
}

vk::QueryPool VKQueryHeap::GetQueryPool() const
{
    return query_pool_.get();
}
