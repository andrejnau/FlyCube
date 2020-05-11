#include "View/DXViewCreater.h"
#include <Utilities/State.h>
#include <Shader/DXReflector.h>
#include <Device/DXDevice.h>
#include <memory>
#include "DXViewDescCreater.h"

DXViewCreater::DXViewCreater(DXDevice& device)
    : m_device(device)
{
}

std::shared_ptr<DXDescriptorHandle> DXViewCreater::GetEmptyDescriptor(ResourceType res_type)
{
    static std::map<ResourceType, std::shared_ptr<DXDescriptorHandle>> empty_handles;
    auto it = empty_handles.find(res_type);
    if (it == empty_handles.end())
        it = empty_handles.emplace(res_type, m_device.GetViewPool().AllocateDescriptor(res_type)).first;
    return std::static_pointer_cast<DXDescriptorHandle>(it->second);
}

std::shared_ptr<DXDescriptorHandle> DXViewCreater::GetView(const std::shared_ptr <Resource>& resource, const ViewDesc& view_desc)
{
    if (!resource)
        return GetEmptyDescriptor(view_desc.res_type);

    DXResource& dx_resource = static_cast<DXResource&>(*resource);
    std::shared_ptr<DXDescriptorHandle> handle = m_device.GetViewPool().AllocateDescriptor(view_desc.res_type);

    switch (view_desc.res_type)
    {
    case ResourceType::kSrv:
        CreateSrv(view_desc, dx_resource, *handle);
        break;
    case ResourceType::kUav:
        CreateUAV(view_desc, dx_resource, *handle);
        break;
    case ResourceType::kCbv:
        CreateCBV(view_desc, dx_resource, *handle);
        break;
    case ResourceType::kSampler:
        CreateSampler(view_desc, dx_resource, *handle);
        break;
    case ResourceType::kRtv:
        CreateRTV(view_desc, dx_resource, *handle);
        break;
    case ResourceType::kDsv:
        CreateDSV(view_desc, dx_resource, *handle);
        break;
    }

    return handle;
}

void DXViewCreater::CreateSrv(const ViewDesc& view_desc, const DXResource& res, DXDescriptorHandle& handle)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = DX12GeSRVDesc(view_desc, res.default_res->GetDesc());
    if (srv_desc.ViewDimension != D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE)
    {
        m_device.GetDevice()->CreateShaderResourceView(res.default_res.Get(), &srv_desc, handle.GetCpuHandle());
    }
    else
    {
        srv_desc.RaytracingAccelerationStructure.Location = res.default_res->GetGPUVirtualAddress();
        m_device.GetDevice()->CreateShaderResourceView(nullptr, &srv_desc, handle.GetCpuHandle());
    }
}

void DXViewCreater::CreateUAV(const ViewDesc& view_desc, const DXResource& res, DXDescriptorHandle& handle)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = DX12GetUAVDesc(view_desc, res.default_res->GetDesc());
    m_device.GetDevice()->CreateUnorderedAccessView(res.default_res.Get(), nullptr, &uav_desc, handle.GetCpuHandle());
}

void DXViewCreater::CreateRTV(const ViewDesc& view_desc, const DXResource& res, DXDescriptorHandle& handle)
{
    D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = DX12GetRTVDesc(view_desc, res.default_res->GetDesc());
    m_device.GetDevice()->CreateRenderTargetView(res.default_res.Get(), &rtv_desc, handle.GetCpuHandle());
}

void DXViewCreater::CreateDSV(const ViewDesc& view_desc, const DXResource& res, DXDescriptorHandle& handle)
{
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = DX12GetDSVDesc(view_desc, res.default_res->GetDesc());
    m_device.GetDevice()->CreateDepthStencilView(res.default_res.Get(), &dsv_desc, handle.GetCpuHandle());
}

void DXViewCreater::CreateCBV(const ViewDesc& view_desc, const DXResource& res, DXDescriptorHandle& handle)
{
    D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
    desc.BufferLocation = res.default_res->GetGPUVirtualAddress();
    desc.SizeInBytes = (res.default_res->GetDesc().Width + 255) & ~255;
    m_device.GetDevice()->CreateConstantBufferView(&desc, handle.GetCpuHandle());
}

void DXViewCreater::CreateSampler(const ViewDesc& view_desc, const DXResource& res, DXDescriptorHandle& handle)
{
    m_device.GetDevice()->CreateSampler(&res.sampler_desc, handle.GetCpuHandle());
}
