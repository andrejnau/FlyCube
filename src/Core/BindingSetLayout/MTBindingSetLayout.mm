#include "BindingSetLayout/MTBindingSetLayout.h"
#include <Device/MTDevice.h>

MTBindingSetLayout::MTBindingSetLayout(MTDevice& device, const std::vector<BindKey>& descs)
    : m_device(device)
{
}
