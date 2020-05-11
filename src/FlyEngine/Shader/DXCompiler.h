#pragma once

#include "Shader/SpirvCompiler.h"
#include <Utilities/FileUtility.h>
#include <Utilities/DXUtility.h>
#include <Utilities/State.h>

#include <d3dcompiler.h>
#include <wrl.h>
#include <assert.h>

using namespace Microsoft::WRL;

struct DXOption
{
    bool spirv = false;
    bool spirv_invert_y = true;
};

ComPtr<ID3DBlob> DXCompile(const ShaderDesc& shader, const DXOption& option = {});
