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

private:
    ComPtr<ID3DBlob> m_blob;
};
