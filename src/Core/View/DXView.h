#pragma once
#include "CPUDescriptorPool/DXCPUDescriptorHandle.h"
#include "GPUDescriptorPool/DXGPUDescriptorPoolRange.h"
#include "Resource/DXResource.h"
#include "View/View.h"

class DXDevice;
class DXResource;

class DXView : public View {
public:
    DXView(DXDevice& device, const std::shared_ptr<DXResource>& resource, const ViewDesc& view_desc);
    std::shared_ptr<Resource> GetResource() override;
    uint32_t GetDescriptorId() const override;
    uint32_t GetBaseMipLevel() const override;
    uint32_t GetLevelCount() const override;
    uint32_t GetBaseArrayLayer() const override;
    uint32_t GetLayerCount() const override;

    D3D12_CPU_DESCRIPTOR_HANDLE GetHandle();

private:
    void CreateView();
    void CreateSRV();
    void CreateRAS();
    void CreateUAV();
    void CreateRTV();
    void CreateDSV();
    void CreateCBV();
    void CreateSampler();

    DXDevice& m_device;
    std::shared_ptr<DXResource> m_resource;
    ViewDesc m_view_desc;
    std::shared_ptr<DXCPUDescriptorHandle> m_handle;
    std::shared_ptr<DXGPUDescriptorPoolRange> m_range;
};
