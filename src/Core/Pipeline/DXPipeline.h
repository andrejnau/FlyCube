#pragma once
#include "Pipeline/Pipeline.h"

#include <directx/d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXPipeline : public Pipeline {
public:
    virtual ~DXPipeline() = default;
    virtual const ComPtr<ID3D12RootSignature>& GetRootSignature() const = 0;
    std::vector<uint8_t> GetRayTracingShaderGroupHandles(uint32_t first_group, uint32_t group_count) const override;
};
