#pragma once

#include "Program/ShaderType.h"
#include <memory>

#include <Program/ShaderType.h>
#include <Utilities/FileUtility.h>
#include <Utilities/DXUtility.h>
#include <map>
#include <vector>

#include <d3dcompiler.h>
#include <wrl.h>
#include <assert.h>

using namespace Microsoft::WRL;

class Context;
class SamplerDesc;

enum class ResourceType
{
    kSrv,
    kUav,
    kCbv,
    kSampler
};

class ProgramApi
{
public:
    virtual void UseProgram(size_t draw_calls) = 0;
    virtual void UpdateData(ComPtr<IUnknown>, const void* ptr) = 0;
    virtual void OnCompileShader(ShaderType type, const ComPtr<ID3DBlob>& blob) = 0;
    virtual void SetDescriptorCount(ShaderType shader_type, ResourceType res_type, uint32_t slot, size_t count) {}
    virtual void AttachSRV(ShaderType type, const std::string& name, uint32_t slot, const ComPtr<IUnknown>& res) = 0;
    virtual void AttachUAV(ShaderType type, const std::string& name, uint32_t slot, const ComPtr<IUnknown>& res) = 0;
    virtual void AttachSampler(ShaderType type, uint32_t slot, const SamplerDesc& desc) = 0;
    virtual void AttachCBuffer(ShaderType type, uint32_t slot, const ComPtr<IUnknown>& res) = 0;
    virtual Context& GetContext() = 0;
};
