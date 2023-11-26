#pragma once
#include "Device/Device.h"
#include "Instance/QueryInterface.h"

#include <memory>
#include <string>

class Adapter : public QueryInterface {
public:
    virtual ~Adapter() = default;
    virtual const std::string& GetName() const = 0;
    virtual std::shared_ptr<Device> CreateDevice() = 0;
};
