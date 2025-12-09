#include "Resource/MTAccelerationStructure.h"

#include "Device/MTDevice.h"
#include "Utilities/Logging.h"

MTAccelerationStructure::MTAccelerationStructure(PassKey<MTAccelerationStructure> pass_key, MTDevice& device)
    : device_(device)
{
}

// static
std::shared_ptr<MTAccelerationStructure> MTAccelerationStructure::CreateAccelerationStructure(
    MTDevice& device,
    const AccelerationStructureDesc& desc)
{
    id<MTLAccelerationStructure> acceleration_structure =
        [device.GetDevice() newAccelerationStructureWithSize:desc.size];
    if (!acceleration_structure) {
        Logging::Println("Failed to create MTLAccelerationStructure");
        return nullptr;
    }

    std::shared_ptr<MTAccelerationStructure> self =
        std::make_shared<MTAccelerationStructure>(PassKey<MTAccelerationStructure>(), device);
    self->resource_type_ = ResourceType::kAccelerationStructure;
    self->acceleration_structure_ = acceleration_structure;
    return self;
}

uint64_t MTAccelerationStructure::GetAccelerationStructureHandle() const
{
    return acceleration_structure_.gpuResourceID._impl;
}

void MTAccelerationStructure::SetName(const std::string& name)
{
    acceleration_structure_.label = [NSString stringWithUTF8String:name.c_str()];
}

id<MTLAccelerationStructure> MTAccelerationStructure::GetAccelerationStructure() const
{
    return acceleration_structure_;
}
