#include "Resource/DXAccelerationStructure.h"

#include "Device/DXDevice.h"

DXAccelerationStructure::DXAccelerationStructure(PassKey<DXAccelerationStructure> pass_key, DXDevice& device)
    : device_(device)
{
}

// static
std::shared_ptr<DXAccelerationStructure> DXAccelerationStructure::CreateAccelerationStructure(
    DXDevice& device,
    const AccelerationStructureDesc& desc)
{
    std::shared_ptr<DXAccelerationStructure> self =
        std::make_shared<DXAccelerationStructure>(PassKey<DXAccelerationStructure>(), device);
    self->resource_type_ = ResourceType::kAccelerationStructure;
    self->acceleration_structure_address_ =
        desc.buffer->As<DXResource>().GetResource()->GetGPUVirtualAddress() + desc.buffer_offset;
    return self;
}

uint64_t DXAccelerationStructure::GetAccelerationStructureHandle() const
{
    return GetAccelerationStructureAddress();
}

void DXAccelerationStructure::SetName(const std::string& name)
{
    // AccelerationStructure does not have name in DirectX 12
}

D3D12_GPU_VIRTUAL_ADDRESS DXAccelerationStructure::GetAccelerationStructureAddress() const
{
    return acceleration_structure_address_;
}
