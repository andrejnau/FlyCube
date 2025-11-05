#include "Instance/MTInstance.h"

#include "Adapter/MTAdapter.h"

MTInstance::MTInstance() = default;

std::vector<std::shared_ptr<Adapter>> MTInstance::EnumerateAdapters()
{
#if TARGET_OS_OSX
    std::vector<std::shared_ptr<Adapter>> adapters;
    NSArray<id<MTLDevice>>* devices = MTLCopyAllDevices();
    for (id<MTLDevice> device : devices) {
        adapters.emplace_back(std::make_shared<MTAdapter>(*this, device));
    }
    return adapters;
#else
    return { std::make_shared<MTAdapter>(*this, MTLCreateSystemDefaultDevice()) };
#endif
}
