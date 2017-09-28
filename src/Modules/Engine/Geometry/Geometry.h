#pragma once

#include <string>
#include <memory>

class Geometry
{
public:
    using Ptr = std::shared_ptr<Geometry>;
    virtual void LoadFromFile(const std::string& path) = 0;
};
