#pragma once
#include <Instance/QueryInterface.h>
#include <Instance/BaseTypes.h>
#include <memory>

class Resource;

class View : public QueryInterface
{
public:
    virtual ~View() = default;
    virtual std::shared_ptr<Resource> GetResource() = 0;
};
