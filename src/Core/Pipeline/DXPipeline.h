#pragma once
#include "Pipeline/Pipeline.h"
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXPipeline : public Pipeline
{
public:
    virtual ~DXPipeline() = default;
    virtual const ComPtr<ID3D12RootSignature>& GetRootSignature() const = 0;
};
