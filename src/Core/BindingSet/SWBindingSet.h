#pragma once
#include "BindingSet/BindingSet.h"

class SWDevice;
class SWBindingSetLayout;

class SWBindingSet
    : public BindingSet
{
public:
    SWBindingSet(SWDevice& device, const std::shared_ptr<SWBindingSetLayout>& layout);

    void WriteBindings(const std::vector<BindingDesc>& bindings) override;

private:
    SWDevice& m_device;
    std::shared_ptr<SWBindingSetLayout> m_layout;
};
