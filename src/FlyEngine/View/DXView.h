#pragma once
#include "View/View.h"
#include <Resource/DXResource.h>
#include <DescriptorPool/DXDescriptorHandle.h>

class DXDevice;

class DXView : public View
{
public:
    DXView(DXDevice& device, const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc);
    D3D12_CPU_DESCRIPTOR_HANDLE GetHandle();

private:
    void CreateSrv(const ViewDesc& view_desc, const DXResource& res, DXDescriptorHandle& handle);
    void CreateUAV(const ViewDesc& view_desc, const DXResource& res, DXDescriptorHandle& handle);
    void CreateRTV(const ViewDesc& view_desc, const DXResource& res, DXDescriptorHandle& handle);
    void CreateDSV(const ViewDesc& view_desc, const DXResource& res, DXDescriptorHandle& handle);
    void CreateCBV(const ViewDesc& view_desc, const DXResource& res, DXDescriptorHandle& handle);
    void CreateSampler(const ViewDesc& view_desc, const DXResource& res, DXDescriptorHandle& handle);

    DXDevice& m_device;
    std::shared_ptr<DXDescriptorHandle> m_handle;
};
