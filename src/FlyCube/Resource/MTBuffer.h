#pragma once
#include "Resource/MTResource.h"
#include "Utilities/PassKey.h"

#import <Metal/Metal.h>

class MTDevice;

class MTBuffer : public MTResource {
public:
    MTBuffer(PassKey<MTBuffer> pass_key, MTDevice& device);

    static std::shared_ptr<MTBuffer> CreateBuffer(MTDevice& device, const BufferDesc& desc);

    void CommitMemory(MemoryType memory_type);
    void BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset);
    MemoryRequirements GetMemoryRequirements() const;

    // Resource:
    uint64_t GetWidth() const override;
    void SetName(const std::string& name) override;
    uint8_t* Map() override;
    void Unmap() override;

    // MTResource:
    id<MTLBuffer> GetBuffer() const override;

private:
    MTDevice& device_;

    id<MTLBuffer> buffer_ = nullptr;
    uint64_t buffer_size = 0;
};
