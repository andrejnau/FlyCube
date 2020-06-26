#pragma once
#include <Instance/BaseTypes.h>
#include <d3dcommon.h>
#include <wrl.h>
using namespace Microsoft::WRL;

struct DXOption
{
    bool spirv = false;
};

ComPtr<ID3DBlob> DXCompile(const ShaderDesc& shader, const DXOption& option = {});
