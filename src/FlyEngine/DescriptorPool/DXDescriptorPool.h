#pragma once
#include "DescriptorPool/DXDescriptorHandle.h"
#include "DescriptorPool/DXDescriptorPoolTyped.h"
#include <Instance/BaseTypes.h>
#include <Utilities/DXUtility.h>
#include <algorithm>
#include <map>
#include <memory>
#include <wrl.h>
#include <d3d12.h>
using namespace Microsoft::WRL;

class DXDevice;

class DXDescriptorPool
{
public:
    DXDescriptorPool(DXDevice& device);
    std::shared_ptr<DXDescriptorHandle> AllocateDescriptor(ResourceType res_type);

private:
    DXDescriptorPoolTyped& SelectHeap(ResourceType res_type);

    DXDevice& m_device;
    DXDescriptorPoolTyped m_resource;
    DXDescriptorPoolTyped m_sampler;
    DXDescriptorPoolTyped m_rtv;
    DXDescriptorPoolTyped m_dsv;
};
