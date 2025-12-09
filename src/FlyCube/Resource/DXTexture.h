#pragma once
#include "Resource/DXResource.h"
#include "Utilities/PassKey.h"

#if defined(_WIN32)
#include <wrl.h>
#else
#include <wsl/wrladapter.h>
#endif

#include <directx/d3d12.h>

using Microsoft::WRL::ComPtr;

class DXDevice;

class DXTexture : public DXResource {
public:
    DXTexture(PassKey<DXTexture> pass_key, DXDevice& device);

    static std::shared_ptr<DXTexture> WrapSwapchainBackBuffer(DXDevice& device,
                                                              ComPtr<ID3D12Resource> back_buffer,
                                                              gli::format format);
    static std::shared_ptr<DXTexture> CreateTexture(DXDevice& device, const TextureDesc& desc);

    void CommitMemory(MemoryType memory_type);
    void BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset);
    MemoryRequirements GetMemoryRequirements() const;

    // Resource:
    uint64_t GetWidth() const override;
    uint32_t GetHeight() const override;
    uint16_t GetLayerCount() const override;
    uint16_t GetLevelCount() const override;
    uint32_t GetSampleCount() const override;
    void SetName(const std::string& name) override;

    // DXResource:
    ID3D12Resource* GetResource() const override;
    const D3D12_RESOURCE_DESC& GetResourceDesc() const override;

private:
    DXDevice& device_;

    ComPtr<ID3D12Resource> resource_;
    D3D12_RESOURCE_DESC resource_desc_ = {};
};
