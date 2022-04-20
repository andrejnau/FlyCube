#pragma once
#include <memory>

class AutoreleasePool
{
public:
    virtual ~AutoreleasePool() = default;
    virtual void Reset() = 0;     
};

std::shared_ptr<AutoreleasePool> CreateAutoreleasePool();
