#pragma once

#include <memory>

class View
{
public:
    virtual ~View() = default;
    using Ptr = std::shared_ptr<View>;
};
