#include "BindingSet/SWBindingSet.h"
#include <Device/SWDevice.h>
#include <BindingSetLayout/SWBindingSetLayout.h>

SWBindingSet::SWBindingSet(SWDevice& device, const std::shared_ptr<SWBindingSetLayout>& layout)
    : m_device(device)
    , m_layout(layout)
{
}

void SWBindingSet::WriteBindings(const std::vector<BindingDesc>& bindings)
{
}
