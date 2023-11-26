#pragma once
#include "BindingSetLayout/BindingSetLayout.h"

class SWDevice;

class SWBindingSetLayout
    : public BindingSetLayout
{
public:
    SWBindingSetLayout(SWDevice& device, const std::vector<BindKey>& descs);

    const std::vector<BindKey>& GetBindKeys() const;

private:
    SWDevice& m_device;
    std::vector<BindKey> m_descs;
};
