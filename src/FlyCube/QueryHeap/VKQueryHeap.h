#pragma once
#include "QueryHeap/QueryHeap.h"

#include <vulkan/vulkan.hpp>

class VKDevice;

class VKQueryHeap : public QueryHeap {
public:
    VKQueryHeap(VKDevice& device, QueryHeapType type, uint32_t count);

    QueryHeapType GetType() const override;

    vk::QueryType GetQueryType() const;
    vk::QueryPool GetQueryPool() const;

private:
    VKDevice& device_;
    vk::UniqueQueryPool query_pool_;
    vk::QueryType query_type_;
};
