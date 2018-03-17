#pragma once

#include <dxgi1_4.h>
#include <wrl.h>

#include <Utilities/DXUtility.h>

using namespace Microsoft::WRL;

ComPtr<IDXGIAdapter1> GetHardwareAdapter(const ComPtr<IDXGIFactory4>& dxgi_factory);
ComPtr<IDXGISwapChain3> CreateSwapChain(const ComPtr<IUnknown>& device, const ComPtr<IDXGIFactory4>& dxgi_factory, HWND window, UINT width, UINT height, UINT FrameCount);
