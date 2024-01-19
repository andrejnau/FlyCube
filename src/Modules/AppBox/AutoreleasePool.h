#pragma once
#include <memory>

class AutoreleasePool {
public:
    virtual ~AutoreleasePool() = default;
    virtual void Reset() {}
};

std::shared_ptr<AutoreleasePool> CreateAutoreleasePool();
