#pragma once
#include "Pipeline/Pipeline.h"

#if defined(_WIN32)
#include <wrl.h>
#else
#include <wsl/wrladapter.h>
#endif

#include <directx/d3d12.h>

using Microsoft::WRL::ComPtr;

class DXPipeline : public Pipeline {
public:
    virtual ~DXPipeline() = default;
    virtual const ComPtr<ID3D12RootSignature>& GetRootSignature() const = 0;
    std::vector<uint8_t> GetRayTracingShaderGroupHandles(uint32_t first_group, uint32_t group_count) const override;
};
