#include "Resource/VKAccelerationStructure.h"

#include "Device/VKDevice.h"
#include "Utilities/NotReached.h"

namespace {

vk::AccelerationStructureTypeKHR Convert(AccelerationStructureType type)
{
    switch (type) {
    case AccelerationStructureType::kTopLevel:
        return vk::AccelerationStructureTypeKHR::eTopLevel;
    case AccelerationStructureType::kBottomLevel:
        return vk::AccelerationStructureTypeKHR::eBottomLevel;
    default:
        NOTREACHED();
    }
}

} // namespace

VKAccelerationStructure::VKAccelerationStructure(PassKey<VKAccelerationStructure> pass_key, VKDevice& device)
    : device_(device)
{
}

// static
std::shared_ptr<VKAccelerationStructure> VKAccelerationStructure::CreateAccelerationStructure(
    VKDevice& device,
    const AccelerationStructureDesc& desc)
{
    vk::AccelerationStructureCreateInfoKHR acceleration_structure_create_info = {};
    acceleration_structure_create_info.buffer = desc.buffer->As<VKAccelerationStructure>().GetBuffer();
    acceleration_structure_create_info.offset = desc.buffer_offset;
    acceleration_structure_create_info.size = desc.size;
    acceleration_structure_create_info.type = Convert(desc.type);

    std::shared_ptr<VKAccelerationStructure> self =
        std::make_shared<VKAccelerationStructure>(PassKey<VKAccelerationStructure>(), device);
    self->resource_type_ = ResourceType::kAccelerationStructure;
    self->acceleration_structure_ =
        device.GetDevice().createAccelerationStructureKHRUnique(acceleration_structure_create_info);
    return self;
}

uint64_t VKAccelerationStructure::GetAccelerationStructureHandle() const
{
    return device_.GetDevice().getAccelerationStructureAddressKHR({ GetAccelerationStructure() });
}

void VKAccelerationStructure::SetName(const std::string& name)
{
    vk::DebugUtilsObjectNameInfoEXT info = {};
    info.pObjectName = name.c_str();
    info.objectType = GetAccelerationStructure().objectType;
    info.objectHandle = reinterpret_cast<uint64_t>(static_cast<VkAccelerationStructureKHR>(GetAccelerationStructure()));
    device_.GetDevice().setDebugUtilsObjectNameEXT(info);
}

vk::AccelerationStructureKHR VKAccelerationStructure::GetAccelerationStructure() const
{
    return acceleration_structure_.get();
}
