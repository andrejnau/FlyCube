#include "BindingSetLayout/MTBindingSetLayout.h"

#include "Device/MTDevice.h"

MTBindingSetLayout::MTBindingSetLayout(MTDevice& device, const BindingSetLayoutDesc& desc)
    : device_(device)
    , bind_keys_(desc.bind_keys)
    , constants_(desc.constants)
{
}

const std::vector<BindKey>& MTBindingSetLayout::GetBindKeys() const
{
    return bind_keys_;
}

const std::vector<BindingConstants>& MTBindingSetLayout::GetConstants() const
{
    return constants_;
}
