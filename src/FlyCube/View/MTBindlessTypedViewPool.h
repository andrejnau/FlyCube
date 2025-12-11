#pragma once
#include "Instance/BaseTypes.h"
#include "View/BindlessTypedViewPool.h"

class MTDevice;
class MTGPUArgumentBufferRange;
class MTView;

class MTBindlessTypedViewPool : public BindlessTypedViewPool {
public:
    MTBindlessTypedViewPool(MTDevice& device, ViewType view_type, uint32_t view_count);

    uint32_t GetBaseDescriptorId() const override;
    uint32_t GetViewCount() const override;
    void WriteView(uint32_t index, const std::shared_ptr<View>& view) override;

    void WriteViewImpl(uint32_t index, MTView& view);

private:
    uint32_t view_count_;
    std::shared_ptr<MTGPUArgumentBufferRange> range_;
};
