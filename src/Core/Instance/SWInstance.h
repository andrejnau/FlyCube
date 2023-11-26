#pragma once
#include "Instance.h"

class SWInstance : public Instance {
public:
    SWInstance();
    std::vector<std::shared_ptr<Adapter>> EnumerateAdapters() override;
};
