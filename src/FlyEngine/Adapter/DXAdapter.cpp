#include "Adapter/DXAdapter.h"
#include <Device/DXDevice.h>
#include <Utilities/DXUtility.h>
#include <Utilities/FileUtility.h>
#include <dxgi1_6.h>
#include <d3d12.h>

DXAdapter::DXAdapter(DXInstance& instance, const ComPtr<IDXGIAdapter1>& adapter)
    : m_instance(instance)
    , m_adapter(adapter)
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
    return std::make_unique<DXDevice>(*this);
}

DXInstance& DXAdapter::GetInstance()
{
    return m_instance;
}

ComPtr<IDXGIAdapter1> DXAdapter::GetAdapter()
{
    return m_adapter;
}
