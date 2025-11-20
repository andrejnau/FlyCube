#pragma once
#include "CPUDescriptorPool/DXCPUDescriptorHandle.h"
#include "CPUDescriptorPool/DXCPUDescriptorPoolTyped.h"
#include "Instance/BaseTypes.h"
#include "Utilities/DXUtility.h"

#if defined(_WIN32)
#include <wrl.h>
#else
#include <wsl/wrladapter.h>
#endif

#include <directx/d3d12.h>

#include <algorithm>
#include <map>
#include <memory>

using Microsoft::WRL::ComPtr;

class DXDevice;

class DXCPUDescriptorPool {
public:
    DXCPUDescriptorPool(DXDevice& device);
    std::shared_ptr<DXCPUDescriptorHandle> AllocateDescriptor(ViewType view_type);

private:
    DXCPUDescriptorPoolTyped& SelectHeap(ViewType view_type);

    DXDevice& device_;
    DXCPUDescriptorPoolTyped resource_;
    DXCPUDescriptorPoolTyped sampler_;
    DXCPUDescriptorPoolTyped rtv_;
    DXCPUDescriptorPoolTyped dsv_;
};
