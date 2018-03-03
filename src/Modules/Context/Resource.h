#pragma once

#include <memory>
#include <string>
#include <vector>
#include <d3d11.h>
#include <d3d12.h>

#include <wrl.h>
using namespace Microsoft::WRL;

#include <Utilities/FileUtility.h>

enum BindFlag
{
    kRtv = 1 << 1,
    kDsv = 1 << 2,
    kSrv = 1 << 3,
    kUav = 1 << 4,
    kCbv = 1 << 5,
    kIbv = 1 << 6,
    kVbv = 1 << 7,
    kSampler = 1 << 8,
};

class Resource
{
public:
    virtual ~Resource() = default;
    virtual void SetName(const std::string& name) = 0;
    using Ptr = std::shared_ptr<Resource>;
};

class DX11Resource : public Resource
{
public:
    using Ptr = std::shared_ptr<DX11Resource>;
    ComPtr<ID3D11Resource> resource;

    virtual void SetName(const std::string& name) override
    {
        resource->SetPrivateData(WKPDID_D3DDebugObjectName, name.size(), name.c_str());
    }
};

class DX12Resource : public Resource
{
public:
    using Ptr = std::shared_ptr<DX12Resource>;
    ComPtr<ID3D12Resource> default_res;
    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
    size_t stride = 0;

    ComPtr<ID3D12Resource>& GetUploadResource(size_t subresource)
    {
        if (subresource >= upload_res.size())
            upload_res.resize(subresource + 1);
        return upload_res[subresource];
    }

    virtual void SetName(const std::string& name) override
    {
        default_res->SetName(utf8_to_wstring(name).c_str());
    }

private:
    std::vector<ComPtr<ID3D12Resource>> upload_res;
};
