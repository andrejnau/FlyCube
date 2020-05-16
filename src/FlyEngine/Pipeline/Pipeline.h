#pragma once
#include <Instance/QueryInterface.h>
#include <memory>

class Pipeline : public QueryInterface
{
public:
    virtual ~Pipeline() = default;
};
