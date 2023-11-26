#pragma once
#include "BindingSetLayout/BindingSetLayout.h"

class MTDevice;

class MTBindingSetLayout : public BindingSetLayout {
public:
    MTBindingSetLayout(MTDevice& device, const std::vector<BindKey>& descs);

    const std::vector<BindKey>& GetBindKeys() const;

private:
    MTDevice& m_device;
    std::vector<BindKey> m_descs;
};
