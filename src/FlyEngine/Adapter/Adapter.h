#pragma once
#include <Device/Device.h>
#include <string>
#include <memory>

class Adapter
{
public:
    virtual ~Adapter() = default;
    virtual const std::string& GetName() const = 0;
    virtual std::shared_ptr<Device> CreateDevice() = 0;
};
