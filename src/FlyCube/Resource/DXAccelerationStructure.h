#pragma once
#include "Resource/DXResource.h"
#include "Utilities/PassKey.h"

#if !defined(_WIN32)
#include <wsl/winadapter.h>
#endif

#include <directx/d3d12.h>

class DXDevice;

class DXAccelerationStructure : public DXResource {
public:
    DXAccelerationStructure(PassKey<DXAccelerationStructure> pass_key, DXDevice& device);

    static std::shared_ptr<DXAccelerationStructure> CreateAccelerationStructure(DXDevice& device,
                                                                                const AccelerationStructureDesc& desc);
    // Resource:
    uint64_t GetAccelerationStructureHandle() const override;
    void SetName(const std::string& name) override;

    // DXResource:
    D3D12_GPU_VIRTUAL_ADDRESS GetAccelerationStructureAddress() const override;

private:
    DXDevice& device_;

    D3D12_GPU_VIRTUAL_ADDRESS acceleration_structure_address_ = {};
};
