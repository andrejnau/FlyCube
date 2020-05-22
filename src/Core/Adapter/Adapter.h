#pragma once
#include <Instance/QueryInterface.h>
#include <Device/Device.h>
#include <string>
#include <memory>

class Adapter : public QueryInterface
{
public:
    virtual ~Adapter() = default;
    virtual const std::string& GetName() const = 0;
    virtual std::shared_ptr<Device> CreateDevice() = 0;
};
