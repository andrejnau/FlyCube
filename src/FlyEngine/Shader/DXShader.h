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
    ShaderType GetType() const override;

    ComPtr<ID3DBlob> GetBlob() const;

private:
    ShaderType m_type;
    ComPtr<ID3DBlob> m_blob;
};
