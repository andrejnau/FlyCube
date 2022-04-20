#include "Utilities/AutoreleasePool.h"
#import <Foundation/NSAutoreleasePool.h>

class AutoreleasePoolImpl : public AutoreleasePool
{
public:
    ~AutoreleasePoolImpl()
    {
        Drain();
    }

    void Reset() override
    {
        Drain();
        m_autorelease_pool = [[NSAutoreleasePool alloc] init];
    }

private:
    void Drain()
    {
        if (m_autorelease_pool)
        {
            [m_autorelease_pool drain];
            m_autorelease_pool = nullptr;
        }
    }

    NSAutoreleasePool* m_autorelease_pool = nullptr;
};

std::shared_ptr<AutoreleasePool> CreateAutoreleasePool()
{
    return std::make_shared<AutoreleasePoolImpl>();
}
