#pragma once
#include "BindingSetLayout/BindingSetLayout.h"

class MTDevice;

class MTBindingSetLayout : public BindingSetLayout {
public:
    MTBindingSetLayout(MTDevice& device, const BindingSetLayoutDesc& desc);

    const std::vector<BindKey>& GetBindKeys() const;
    const std::vector<BindingConstants>& GetConstants() const;

private:
    MTDevice& device_;
    std::vector<BindKey> bind_keys_;
    std::vector<BindingConstants> constants_;
};
