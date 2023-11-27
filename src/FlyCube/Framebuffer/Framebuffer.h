#pragma once
#include "Instance/QueryInterface.h"

#include <memory>

class Framebuffer : public QueryInterface {
public:
    virtual ~Framebuffer() = default;
};
