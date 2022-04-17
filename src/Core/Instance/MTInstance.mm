#include "Instance/MTInstance.h"
#include <Adapter/MTAdapter.h>
#import <Metal/Metal.h>

MTInstance::MTInstance()
{
    setenv("METAL_DEBUG_ERROR_MODE", "5", 1);
}

std::vector<std::shared_ptr<Adapter>> MTInstance::EnumerateAdapters()
{
    std::vector<std::shared_ptr<Adapter>> adapters;
    NSArray<id<MTLDevice>>* devices = MTLCopyAllDevices();
    for (id<MTLDevice> device : devices)
    {
        adapters.emplace_back(std::make_shared<MTAdapter>(device));
    }
    return adapters;
}
