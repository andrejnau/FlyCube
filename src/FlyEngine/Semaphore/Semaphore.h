#pragma once
#include <Instance/QueryInterface.h>
#include <cstdint>

class Semaphore : public QueryInterface
{
public:
    virtual ~Semaphore() = default;
};
