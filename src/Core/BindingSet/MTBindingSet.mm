#include "BindingSet/MTBindingSet.h"
#include <Device/MTDevice.h>
#include <BindingSetLayout/MTBindingSetLayout.h>

MTBindingSet::MTBindingSet(MTDevice& device, const std::shared_ptr<MTBindingSetLayout>& layout)
    : m_device(device)
    , m_layout(layout)
{
}

void MTBindingSet::WriteBindings(const std::vector<BindingDesc>& bindings)
{
}
