#include "Adapter/SWAdapter.h"
#include <Device/SWDevice.h>

SWAdapter::SWAdapter(SWInstance& instance)
    : m_name("SW")
{
}

const std::string& SWAdapter::GetName() const
{
    return m_name;
}

std::shared_ptr<Device> SWAdapter::CreateDevice()
{
    return std::make_shared<SWDevice>(*this);
}
