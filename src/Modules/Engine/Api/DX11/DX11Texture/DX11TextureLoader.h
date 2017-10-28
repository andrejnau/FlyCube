#pragma once

#include "DX11Texture/TextureInfo.h"
#include <d3d11.h>
#include <wrl.h>
using namespace Microsoft::WRL;

ComPtr<ID3D11ShaderResourceView> CreateTexture(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& device_context, TextureInfo& texture);
