#include "Instance/SWInstance.h"

#include "Adapter/SWAdapter.h"

SWInstance::SWInstance() {}

std::vector<std::shared_ptr<Adapter>> SWInstance::EnumerateAdapters()
{
    return { std::make_shared<SWAdapter>(*this) };
}
