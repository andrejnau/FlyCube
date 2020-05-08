#include "Device/Device.h"
#include <dxgi.h>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXDevice : public Device
{
public:
    DXDevice(const ComPtr<IDXGIAdapter1>& adapter);

private:
    ComPtr<ID3D12Device> m_device;
};
