#pragma once
#include <Instance/QueryInterface.h>
#include <Instance/BaseTypes.h>
#include <memory>

class Resource;

class View : public QueryInterface
{
public:
    virtual ~View() = default;
    virtual const std::shared_ptr<Resource>& GetResource() const = 0;
    virtual uint32_t GetDescriptorId() const = 0;
};
