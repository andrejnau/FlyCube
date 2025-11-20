#include "Adapter/DXAdapter.h"

#include "Device/DXDevice.h"
#include "Utilities/DXUtility.h"
#include "Utilities/SystemUtils.h"

#include <nowide/convert.hpp>

DXAdapter::DXAdapter(DXInstance& instance, const ComPtr<IDXGIAdapter1>& adapter)
    : instance_(instance)
    , adapter_(adapter)
{
    DXGI_ADAPTER_DESC desc = {};
    adapter->GetDesc(&desc);
    name_ = nowide::narrow(desc.Description);
}

const std::string& DXAdapter::GetName() const
{
    return name_;
}

std::shared_ptr<Device> DXAdapter::CreateDevice()
{
    return std::make_shared<DXDevice>(*this);
}

DXInstance& DXAdapter::GetInstance()
{
    return instance_;
}

ComPtr<IDXGIAdapter1> DXAdapter::GetAdapter()
{
    return adapter_;
}
