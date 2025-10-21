#pragma once
#include "Resource/ResourceBase.h"
#include "Utilities/Common.h"

#include <directx/d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXDevice;

class DXResource : public ResourceBase {
public:
    DXResource(PassKey<DXResource> pass_key, DXDevice& device);

    static std::shared_ptr<DXResource> WrapSwapchainBackBuffer(DXDevice& device,
                                                               ComPtr<ID3D12Resource> back_buffer,
                                                               gli::format format);

    static std::shared_ptr<DXResource> CreateTexture(DXDevice& device,
                                                     TextureType type,
                                                     uint32_t bind_flag,
                                                     gli::format format,
                                                     uint32_t sample_count,
                                                     int width,
                                                     int height,
                                                     int depth,
                                                     int mip_levels);

    static std::shared_ptr<DXResource> CreateBuffer(DXDevice& device, uint32_t bind_flag, uint32_t buffer_size);

    static std::shared_ptr<DXResource> CreateSampler(DXDevice& device, const SamplerDesc& desc);

    static std::shared_ptr<DXResource> CreateAccelerationStructure(
        DXDevice& device,
        AccelerationStructureType type,
        const std::shared_ptr<Resource>& acceleration_structures_memory,
        uint64_t offset,
        uint64_t size);

    void CommitMemory(MemoryType memory_type) override;
    void BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset) override;
    uint64_t GetWidth() const override;
    uint32_t GetHeight() const override;
    uint16_t GetLayerCount() const override;
    uint16_t GetLevelCount() const override;
    uint32_t GetSampleCount() const override;
    uint64_t GetAccelerationStructureHandle() const override;
    void SetName(const std::string& name) override;
    uint8_t* Map() override;
    void Unmap() override;
    MemoryRequirements GetMemoryRequirements() const override;

    const ComPtr<ID3D12Resource>& GetResource() const;
    const D3D12_RESOURCE_DESC& GetResourceDesc() const;
    const D3D12_SAMPLER_DESC& GetSamplerDesc() const;
    D3D12_GPU_VIRTUAL_ADDRESS GetAccelerationStructureAddress() const;

private:
    DXDevice& m_device;

    ComPtr<ID3D12Resource> m_resource;
    D3D12_RESOURCE_DESC m_resource_desc = {};
    D3D12_SAMPLER_DESC m_sampler_desc = {};
    D3D12_GPU_VIRTUAL_ADDRESS m_acceleration_structure_address = {};
};
