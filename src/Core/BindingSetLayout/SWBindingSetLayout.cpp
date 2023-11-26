#include "BindingSetLayout/SWBindingSetLayout.h"
#include <Device/SWDevice.h>

SWBindingSetLayout::SWBindingSetLayout(SWDevice& device, const std::vector<BindKey>& descs)
    : m_device(device)
    , m_descs(descs)
{
}

const std::vector<BindKey>& SWBindingSetLayout::GetBindKeys() const
{
    return m_descs;
}
