#pragma once
#include "View/View.h"
#include <Resource/DXResource.h>
#include <CPUDescriptorPool/DXCPUDescriptorHandle.h>
#include <GPUDescriptorPool/DXGPUDescriptorPoolRange.h>

class DXDevice;

class DXView : public View
{
public:
    DXView(DXDevice& device, const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc);
    const std::shared_ptr<Resource>& GetResource() const override;
    uint32_t GetDescriptorId() const override;

    D3D12_CPU_DESCRIPTOR_HANDLE GetHandle();
    DXGI_FORMAT GetFormat() const;

private:
    void CreateSrv(const ViewDesc& view_desc, const DXResource* res, DXCPUDescriptorHandle& handle);
    void CreateUAV(const ViewDesc& view_desc, const DXResource* res, DXCPUDescriptorHandle& handle);
    void CreateRTV(const ViewDesc& view_desc, const DXResource* res, DXCPUDescriptorHandle& handle);
    void CreateDSV(const ViewDesc& view_desc, const DXResource* res, DXCPUDescriptorHandle& handle);
    void CreateCBV(const ViewDesc& view_desc, const DXResource* res, DXCPUDescriptorHandle& handle);
    void CreateSampler(const ViewDesc& view_desc, const DXResource* res, DXCPUDescriptorHandle& handle);

    DXDevice& m_device;
    DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;
    std::shared_ptr<DXCPUDescriptorHandle> m_handle;
    std::shared_ptr<Resource> m_resource;
    std::shared_ptr<DXGPUDescriptorPoolRange> m_range;
};
