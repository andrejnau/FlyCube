#pragma once
#include "BindingSet/BindingSet.h"

class MTDevice;
class MTBindingSetLayout;

class MTBindingSet
    : public BindingSet
{
public:
    MTBindingSet(MTDevice& device, const std::shared_ptr<MTBindingSetLayout>& layout);

    void WriteBindings(const std::vector<BindingDesc>& bindings) override;

private:
    MTDevice& m_device;
    std::shared_ptr<MTBindingSetLayout> m_layout;
};
