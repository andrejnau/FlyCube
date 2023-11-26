#include "Adapter/DXAdapter.h"

#include "Device/DXDevice.h"
#include "Utilities/DXUtility.h"
#include "Utilities/SystemUtils.h"

#include <directx/d3d12.h>
#include <dxgi1_6.h>
#include <nowide/convert.hpp>

DXAdapter::DXAdapter(DXInstance& instance, const ComPtr<IDXGIAdapter1>& adapter)
    : m_instance(instance)
    , m_adapter(adapter)
{
    DXGI_ADAPTER_DESC desc = {};
    adapter->GetDesc(&desc);
    m_name = nowide::narrow(desc.Description);
}

const std::string& DXAdapter::GetName() const
{
    return m_name;
}

std::shared_ptr<Device> DXAdapter::CreateDevice()
{
    return std::make_shared<DXDevice>(*this);
}

DXInstance& DXAdapter::GetInstance()
{
    return m_instance;
}

ComPtr<IDXGIAdapter1> DXAdapter::GetAdapter()
{
    return m_adapter;
}
