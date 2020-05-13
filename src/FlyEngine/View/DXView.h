#pragma once
#include "View/View.h"
#include <Resource/DXResource.h>
#include <CPUDescriptorPool/DXCPUDescriptorHandle.h>

class DXDevice;

class DXView : public View
{
public:
    DXView(DXDevice& device, const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc);
    D3D12_CPU_DESCRIPTOR_HANDLE GetHandle();
    DXGI_FORMAT GetFormat() const;

private:
    void CreateSrv(const ViewDesc& view_desc, const DXResource& res, DXCPUDescriptorHandle& handle);
    void CreateUAV(const ViewDesc& view_desc, const DXResource& res, DXCPUDescriptorHandle& handle);
    void CreateRTV(const ViewDesc& view_desc, const DXResource& res, DXCPUDescriptorHandle& handle);
    void CreateDSV(const ViewDesc& view_desc, const DXResource& res, DXCPUDescriptorHandle& handle);
    void CreateCBV(const ViewDesc& view_desc, const DXResource& res, DXCPUDescriptorHandle& handle);
    void CreateSampler(const ViewDesc& view_desc, const DXResource& res, DXCPUDescriptorHandle& handle);

    DXDevice& m_device;
    DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;
    std::shared_ptr<DXCPUDescriptorHandle> m_handle;
};
