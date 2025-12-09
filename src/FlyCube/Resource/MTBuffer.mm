#include "Resource/MTBuffer.h"

#include "Device/MTDevice.h"
#include "Memory/MTMemory.h"
#include "Utilities/Logging.h"

MTBuffer::MTBuffer(PassKey<MTBuffer> pass_key, MTDevice& device)
    : device_(device)
{
}

// static
std::shared_ptr<MTBuffer> MTBuffer::CreateBuffer(MTDevice& device, const BufferDesc& desc)
{
    if (desc.size == 0) {
        return nullptr;
    }

    std::shared_ptr<MTBuffer> self = std::make_shared<MTBuffer>(PassKey<MTBuffer>(), device);
    self->resource_type_ = ResourceType::kBuffer;
    self->buffer_size = desc.size;
    return self;
}

void MTBuffer::CommitMemory(MemoryType memory_type)
{
    memory_type_ = memory_type;
    MTLResourceOptions options = ConvertStorageMode(memory_type_) << MTLResourceStorageModeShift;
    buffer_ = [device_.GetDevice() newBufferWithLength:buffer_size options:options];
    if (!buffer_) {
        Logging::Println("Failed to create MTLBuffer");
    }
}

void MTBuffer::BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset)
{
    memory_type_ = memory->GetMemoryType();
    id<MTLHeap> mt_heap = memory->As<MTMemory>().GetHeap();
    MTLResourceOptions options = ConvertStorageMode(memory_type_) << MTLResourceStorageModeShift;
    buffer_ = [mt_heap newBufferWithLength:buffer_size options:options offset:offset];
    if (!buffer_) {
        Logging::Println("Failed to create MTLBuffer from heap {}", mt_heap);
    }
}

MemoryRequirements MTBuffer::GetMemoryRequirements() const
{
    MTLResourceOptions options = ConvertStorageMode(MemoryType::kDefault) << MTLResourceStorageModeShift;
    MTLSizeAndAlign size_align = [device_.GetDevice() heapBufferSizeAndAlignWithLength:buffer_size options:options];
    return { size_align.size, size_align.align, 0 };
}

uint64_t MTBuffer::GetWidth() const
{
    return buffer_size;
}

void MTBuffer::SetName(const std::string& name)
{
    buffer_.label = [NSString stringWithUTF8String:name.c_str()];
}

uint8_t* MTBuffer::Map()
{
    return static_cast<uint8_t*>(buffer_.contents);
}

void MTBuffer::Unmap() {}

id<MTLBuffer> MTBuffer::GetBuffer() const
{
    return buffer_;
}
