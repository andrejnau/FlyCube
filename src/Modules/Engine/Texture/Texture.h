#pragma once

#include <memory>

class Texture
{
public:
    using Ptr = std::shared_ptr<Texture>;
    virtual ~Texture() = default;
};
