#include "BindingSetLayout/MTBindingSetLayout.h"

#include "Device/MTDevice.h"

MTBindingSetLayout::MTBindingSetLayout(MTDevice& device,
                                       const std::vector<BindKey>& bind_keys,
                                       const std::vector<BindingConstants>& constants)
    : device_(device)
    , bind_keys_(bind_keys)
{
}

const std::vector<BindKey>& MTBindingSetLayout::GetBindKeys() const
{
    return bind_keys_;
}
