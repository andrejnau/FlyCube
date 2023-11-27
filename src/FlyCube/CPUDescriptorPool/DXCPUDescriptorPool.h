#pragma once
#include "CPUDescriptorPool/DXCPUDescriptorHandle.h"
#include "CPUDescriptorPool/DXCPUDescriptorPoolTyped.h"
#include "Instance/BaseTypes.h"
#include "Utilities/DXUtility.h"

#include <directx/d3d12.h>
#include <wrl.h>

#include <algorithm>
#include <map>
#include <memory>
using namespace Microsoft::WRL;

class DXDevice;

class DXCPUDescriptorPool {
public:
    DXCPUDescriptorPool(DXDevice& device);
    std::shared_ptr<DXCPUDescriptorHandle> AllocateDescriptor(ViewType view_type);

private:
    DXCPUDescriptorPoolTyped& SelectHeap(ViewType view_type);

    DXDevice& m_device;
    DXCPUDescriptorPoolTyped m_resource;
    DXCPUDescriptorPoolTyped m_sampler;
    DXCPUDescriptorPoolTyped m_rtv;
    DXCPUDescriptorPoolTyped m_dsv;
};
