#include "Adapter/DXAdapter.h"
#include <Device/DXDevice.h>
#include <Utilities/DXUtility.h>
#include <Utilities/FileUtility.h>
#include <dxgi1_6.h>
#include <d3d12.h>

DXAdapter::DXAdapter(const ComPtr<IDXGIAdapter1>& adapter)
    : m_adapter(adapter)
{
    DXGI_ADAPTER_DESC desc = {};
    adapter->GetDesc(&desc);
    m_name = wstring_to_utf8(desc.Description);
}

const std::string& DXAdapter::GetName() const
{
    return m_name;
}

std::unique_ptr<Device> DXAdapter::CreateDevice()
{
    return std::make_unique<DXDevice>(m_adapter);
}
