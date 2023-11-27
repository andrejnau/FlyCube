#pragma once
#include "Adapter/Adapter.h"

class SWInstance;

class SWAdapter : public Adapter {
public:
    SWAdapter(SWInstance& instance);
    const std::string& GetName() const override;
    std::shared_ptr<Device> CreateDevice() override;

private:
    std::string m_name;
};
