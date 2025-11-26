#pragma once
#include "BindingSetLayout/BindingSetLayout.h"

class MTDevice;

class MTBindingSetLayout : public BindingSetLayout {
public:
    MTBindingSetLayout(MTDevice& device,
                       const std::vector<BindKey>& bind_keys,
                       const std::vector<BindingConstants>& constants);

    const std::vector<BindKey>& GetBindKeys() const;

private:
    MTDevice& device_;
    std::vector<BindKey> bind_keys_;
};
