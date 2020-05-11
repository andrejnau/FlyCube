#pragma once

#include <Resource/DXResource.h>
#include <View/View.h>
#include <Device/DXViewPool.h>

class DXDevice;

class DXViewCreater
{
public:
    DXViewCreater(DXDevice& device);
    std::shared_ptr<DXDescriptorHandle> GetView(const std::shared_ptr <Resource>& resource, const ViewDesc& view_desc);

private:
    DXDevice& m_device;
    std::shared_ptr<DXDescriptorHandle> GetEmptyDescriptor(ResourceType res_type);

    void CreateSrv(const ViewDesc& view_desc, const DXResource& res, DXDescriptorHandle& handle);
    void CreateUAV(const ViewDesc& view_desc, const DXResource& res, DXDescriptorHandle& handle);
    void CreateRTV(const ViewDesc& view_desc, const DXResource& res, DXDescriptorHandle& handle);
    void CreateDSV(const ViewDesc& view_desc, const DXResource& res, DXDescriptorHandle& handle);
    void CreateCBV(const ViewDesc& view_desc, const DXResource& res, DXDescriptorHandle& handle);
    void CreateSampler(const ViewDesc& view_desc, const DXResource& res, DXDescriptorHandle& handle);
};
