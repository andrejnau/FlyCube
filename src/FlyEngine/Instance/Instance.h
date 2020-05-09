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
    virtual std::vector<std::shared_ptr<Adapter>> EnumerateAdapters() = 0;
};

std::shared_ptr<Instance> CreateInstance(ApiType type);
