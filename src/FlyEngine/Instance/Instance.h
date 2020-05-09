#pragma once
#include "Instance/BaseTypes.h"
#include <ApiType/ApiType.h>
#include <Adapter/Adapter.h>
#include <memory>
#include <string>
#include <vector>

class Instance
{
public:
    virtual ~Instance() = default;
    virtual std::vector<std::unique_ptr<Adapter>> EnumerateAdapters() = 0;
};

std::unique_ptr<Instance> CreateInstance(ApiType type);
