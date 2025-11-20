#pragma once
#include "QueryHeap/QueryHeap.h"

#import <Metal/Metal.h>

class MTDevice;

class MTQueryHeap : public QueryHeap {
public:
    MTQueryHeap(MTDevice& device, QueryHeapType type, uint32_t count);

    QueryHeapType GetType() const override;

    id<MTLBuffer> GetBuffer() const;

private:
    id<MTLBuffer> buffer_ = nullptr;
};
