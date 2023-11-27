#pragma once
#include "Adapter/Adapter.h"
#include "ApiType/ApiType.h"
#include "Instance/BaseTypes.h"
#include "Instance/QueryInterface.h"

#include <memory>
#include <string>
#include <vector>

class Instance : public QueryInterface {
public:
    virtual ~Instance() = default;
    virtual std::vector<std::shared_ptr<Adapter>> EnumerateAdapters() = 0;
};

std::shared_ptr<Instance> CreateInstance(ApiType type);
