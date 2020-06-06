#pragma once
#include "Shader/Shader.h"
#include <Instance/BaseTypes.h>
#include <map>
#include <d3dcommon.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXShader : public Shader
{
public:
    DXShader(const ShaderDesc& desc);
    std::vector<VertexInputDesc> GetInputLayout() const override;
    ResourceBindingDesc GetResourceBindingDesc(const BindKey& bind_key) const override;
    uint32_t GetResourceStride(const BindKey& bind_key) const override;
    ShaderType GetType() const override;
    BindKey GetBindKey(const std::string& name) const override;

    ComPtr<ID3DBlob> GetBlob() const;

private:
    ShaderType m_type;
    ComPtr<ID3DBlob> m_blob;
    std::map<BindKey, std::string> m_names;
    std::map<std::string, BindKey> m_bind_keys;
};
