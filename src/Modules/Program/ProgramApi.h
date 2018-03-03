#pragma once

#include "Program/ShaderType.h"
#include <memory>

#include <Program/ShaderType.h>
#include <Context/BaseTypes.h>
#include <Context/Resource.h>
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
class BufferLayout;

class ProgramApi
{
public:
    virtual void SetMaxEvents(size_t count) = 0;
    virtual void UseProgram() = 0;
    virtual void ApplyBindings() = 0;
    virtual void OnCompileShader(ShaderType type, const ComPtr<ID3DBlob>& blob) = 0;
    virtual void AttachSRV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires) = 0;
    virtual void AttachUAV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires) = 0;
    virtual void AttachCBuffer(ShaderType type, UINT slot, BufferLayout& buffer) = 0;
    virtual void AttachSampler(ShaderType type, uint32_t slot, const SamplerDesc& desc) = 0;
};
