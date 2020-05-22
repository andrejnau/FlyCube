#pragma once
#include "GPUDescriptorPool/DXGPUDescriptorPoolRange.h"
#include "GPUDescriptorPool/DXGPUDescriptorPoolTyped.h"
#include <Instance/BaseTypes.h>
#include <Utilities/DXUtility.h>
#include <algorithm>
#include <map>
#include <memory>
#include <wrl.h>
#include <d3d12.h>
using namespace Microsoft::WRL;

class DXDevice;

class DXGPUDescriptorPool
{
public:
    DXGPUDescriptorPool(DXDevice& device);
    DXGPUDescriptorPoolRange Allocate(D3D12_DESCRIPTOR_HEAP_TYPE descriptor_type, size_t count);

private:
    DXDevice& m_device;
    DXGPUDescriptorPoolTyped m_shader_resource;
    DXGPUDescriptorPoolTyped m_shader_sampler;
};
