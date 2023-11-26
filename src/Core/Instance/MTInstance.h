#pragma once
#include "Instance/Instance.h"
#include "Utilities/AutoreleasePool.h"

class MTInstance : public Instance {
public:
    MTInstance();
    std::vector<std::shared_ptr<Adapter>> EnumerateAdapters() override;

    std::shared_ptr<AutoreleasePool> GetAutoreleasePool();

private:
    std::shared_ptr<AutoreleasePool> m_autorelease_pool;
};
