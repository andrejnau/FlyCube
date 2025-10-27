#pragma once
#include "Resource/ResourceBase.h"
#include "Utilities/PassKey.h"

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
    static std::shared_ptr<DXResource> CreateTexture(DXDevice& device, const TextureDesc& desc);
    static std::shared_ptr<DXResource> CreateBuffer(DXDevice& device, const BufferDesc& desc);
    static std::shared_ptr<DXResource> CreateSampler(DXDevice& device, const SamplerDesc& desc);
    static std::shared_ptr<DXResource> CreateAccelerationStructure(DXDevice& device,
                                                                   const AccelerationStructureDesc& desc);

    void CommitMemory(MemoryType memory_type);
    void BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset);
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

    ID3D12Resource* GetResource() const;
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
