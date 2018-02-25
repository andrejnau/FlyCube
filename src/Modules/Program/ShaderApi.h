#pragma once

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

class ShaderApi
{
protected:
    
public:
    virtual ~ShaderApi() = default;
    virtual void UpdateData(ComPtr<IUnknown>, const void* ptr) = 0;
    virtual void UseShader() = 0;
    virtual void CreateShader(const ComPtr<ID3DBlob>& blob) = 0;
    virtual void Attach(uint32_t slot, ComPtr<ID3D11ShaderResourceView>& srv) = 0;
    virtual void Attach(uint32_t slot, ComPtr<ID3D11UnorderedAccessView>& uav) = 0;
    virtual void Attach(uint32_t slot, ComPtr<ID3D11SamplerState>& sampler) = 0;
    virtual void AttachSRV(const std::string& name, uint32_t slot, const ComPtr<IUnknown>& res) = 0;
    virtual ComPtr<ID3D11ShaderResourceView> CreateSrv(const std::string& name, uint32_t slot, const ComPtr<IUnknown>& ires) = 0;
    virtual void AttachUAV(const std::string& name, uint32_t slot, const ComPtr<IUnknown>& res) = 0;
    virtual ComPtr<ID3D11UnorderedAccessView> CreateUAV(const std::string& name, uint32_t slot, const ComPtr<IUnknown>& ires) = 0;
    virtual void BindCBuffer(uint32_t slot, const ComPtr<IUnknown>& res) = 0;
    virtual Context& GetContext() = 0;
protected:
    ComPtr<ID3DBlob> m_shader_buffer;
};
