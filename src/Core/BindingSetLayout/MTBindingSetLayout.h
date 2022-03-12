#pragma once
#include "BindingSetLayout/BindingSetLayout.h"

class MTDevice;

class MTBindingSetLayout
    : public BindingSetLayout
{
public:
    MTBindingSetLayout(MTDevice& device, const std::vector<BindKey>& descs);

private:
    MTDevice& m_device;
};
