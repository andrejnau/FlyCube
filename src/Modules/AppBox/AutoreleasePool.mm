#include "AppBox/AutoreleasePool.h"

#import <Foundation/NSAutoreleasePool.h>

class AutoreleasePoolImpl : public AutoreleasePool {
public:
    ~AutoreleasePoolImpl() override
    {
        Drain();
    }

    void Reset() override
    {
        Drain();
        autorelease_pool_ = [NSAutoreleasePool new];
    }

private:
    void Drain()
    {
        if (autorelease_pool_) {
            [autorelease_pool_ drain];
            autorelease_pool_ = nullptr;
        }
    }

    NSAutoreleasePool* autorelease_pool_ = nullptr;
};

std::shared_ptr<AutoreleasePool> CreateAutoreleasePool()
{
#if defined(USE_EXTERNAL_AUTORELEASEPOOL)
    return std::make_shared<AutoreleasePool>();
#else
    return std::make_shared<AutoreleasePoolImpl>();
#endif
}
