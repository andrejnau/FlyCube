#pragma once

#include <d3d12.h>
#include <map>

#include "Resource/Resource.h"
#include "Resource/IDestroyer.h"
#include "Context/BaseTypes.h"
#include <View/DX12View.h>

#include <wrl.h>
using namespace Microsoft::WRL;

class DX12Resource : public Resource
{
public:
    using Destroyer = IDestroyer<ComPtr<IUnknown>>;
    using Ptr = std::shared_ptr<DX12Resource>;
    ComPtr<ID3D12Resource> default_res;
    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
    uint32_t bind_flag = 0;
    uint32_t buffer_size = 0;
    uint32_t stride = 0;
    D3D12_RESOURCE_DESC desc = {};

    D3D12_SAMPLER_DESC sampler_desc = {};

    DX12Resource(Destroyer& destroyer);
    ~DX12Resource();

    struct
    {
        DX12Resource::Ptr scratch;
        DX12Resource::Ptr result;
        DX12Resource::Ptr instance_desc;
    } as;

    virtual void SetName(const std::string& name) override;
    ComPtr<ID3D12Resource>& GetUploadResource(size_t subresource);

private:
    std::reference_wrapper<Destroyer> m_destroyer;
    std::vector<ComPtr<ID3D12Resource>> m_upload_res;
};
