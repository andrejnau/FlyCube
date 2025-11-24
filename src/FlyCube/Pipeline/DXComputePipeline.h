#pragma once
#include "Instance/BaseTypes.h"
#include "Pipeline/DXPipeline.h"

#if defined(_WIN32)
#include <wrl.h>
#else
#include <wsl/wrladapter.h>
#endif

#include <directx/d3d12.h>

using Microsoft::WRL::ComPtr;

class DXDevice;

class DXComputePipeline : public DXPipeline {
public:
    DXComputePipeline(DXDevice& device, const ComputePipelineDesc& desc);
    PipelineType GetPipelineType() const override;
    const ComPtr<ID3D12RootSignature>& GetRootSignature() const override;

    const ComputePipelineDesc& GetDesc() const;
    const ComPtr<ID3D12PipelineState>& GetPipeline() const;

private:
    DXDevice& device_;
    ComputePipelineDesc desc_;
    ComPtr<ID3D12RootSignature> root_signature_;
    ComPtr<ID3D12PipelineState> pipeline_state_;
};
