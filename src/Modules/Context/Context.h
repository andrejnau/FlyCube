#pragma once

#include <d3d11.h>
#include <DXGI1_4.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class Context
{
public:
    Context(int width, int height);
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> device_context;
    ComPtr<IDXGISwapChain> swap_chain;
};
