#include "Device/DXDevice.h"
#include <Utilities/DXUtility.h>
#include <dxgi1_6.h>

DXDevice::DXDevice(const ComPtr<IDXGIAdapter1>& adapter)
{
    ASSERT_SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&m_device)));
}
