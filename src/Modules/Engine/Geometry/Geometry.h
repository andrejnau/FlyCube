#pragma once

#include <string>
#include <memory>

class Geometry
{
public:
    using Ptr = std::shared_ptr<Geometry>;
    virtual ~Geometry() = default;
};

