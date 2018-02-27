#pragma once

#include "Program/ShaderType.h"
#include <memory>

#include <Program/ShaderType.h>
#include <Utilities/FileUtility.h>
#include <Utilities/DXUtility.h>
#include <map>
#include <vector>

#include <d3dcompiler.h>
#include <d3d11.h>
#include <wrl.h>
#include <assert.h>

using namespace Microsoft::WRL;

class Context;

class ProgramApi
{
public:
    virtual void UseProgram() = 0;
    virtual void UpdateData(ComPtr<IUnknown>, const void* ptr) = 0;
    virtual void OnCompileShader(ShaderType type, const ComPtr<ID3DBlob>& blob) = 0;
    virtual void AttachSRV(ShaderType type, const std::string& name, uint32_t slot, const ComPtr<IUnknown>& res) = 0;
    virtual void AttachUAV(ShaderType type, const std::string& name, uint32_t slot, const ComPtr<IUnknown>& res) = 0;
    virtual void AttachRTV(uint32_t slot, const ComPtr<IUnknown>& res) = 0;
    virtual void AttachDSV(const ComPtr<IUnknown>& res) = 0;
    virtual void UpdateOmSet() = 0;
    virtual void AttachSampler(ShaderType type, uint32_t slot, const ComPtr<IUnknown>& res) = 0;
    virtual void AttachCBuffer(ShaderType type, uint32_t slot, const ComPtr<IUnknown>& res) = 0;
    virtual Context& GetContext() = 0;
};
