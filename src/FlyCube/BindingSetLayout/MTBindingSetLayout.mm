#include "BindingSetLayout/MTBindingSetLayout.h"

#include "Device/MTDevice.h"

MTBindingSetLayout::MTBindingSetLayout(MTDevice& device, const std::vector<BindKey>& descs)
    : device_(device)
    , descs_(descs)
{
}

const std::vector<BindKey>& MTBindingSetLayout::GetBindKeys() const
{
    return descs_;
}
