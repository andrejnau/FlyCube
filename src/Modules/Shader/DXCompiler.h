#pragma once

#include "Shader/ShaderBase.h"
#include <Utilities/FileUtility.h> 
#include <Utilities/DXUtility.h> 
#include <Utilities/State.h>

#include <d3dcompiler.h> 
#include <wrl.h> 
#include <assert.h> 

using namespace Microsoft::WRL;

ComPtr<ID3DBlob> DXCompile(const ShaderDesc& shader);
