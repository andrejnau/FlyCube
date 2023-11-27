#include "BindingSetLayout/MTBindingSetLayout.h"

#include "Device/MTDevice.h"

MTBindingSetLayout::MTBindingSetLayout(MTDevice& device, const std::vector<BindKey>& descs)
    : m_device(device)
    , m_descs(descs)
{
}

const std::vector<BindKey>& MTBindingSetLayout::GetBindKeys() const
{
    return m_descs;
}
