#pragma once
#include "Shader/Shader.h"
#include <Instance/BaseTypes.h>
#include <d3dcommon.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXShader : public Shader
{
public:
    DXShader(const ShaderDesc& desc);
    std::vector<VertexInputDesc> GetInputLayout() const override;
    ResourceBindingDesc GetResourceBindingDesc(const std::string& name) const override;
    ShaderType GetType() const override;

    ComPtr<ID3DBlob> GetBlob() const;

private:
    ShaderType m_type;
    ComPtr<ID3DBlob> m_blob;
};
