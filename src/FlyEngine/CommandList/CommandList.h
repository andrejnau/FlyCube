#pragma once
#include <memory>

class Resource;

class CommandList
{
public:
    virtual ~CommandList() = default;
    virtual void Clear(std::shared_ptr<Resource> iresource, const std::array<float, 4>& color) = 0;
};
