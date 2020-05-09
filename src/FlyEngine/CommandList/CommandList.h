#pragma once
#include <Resource/Resource.h>
#include <memory>

class CommandList
{
public:
    virtual ~CommandList() = default;
    virtual void Open() = 0;
    virtual void Close() = 0;
    virtual void Clear(Resource::Ptr resource, const std::array<float, 4>& color) = 0;
    virtual void ResourceBarrier(Resource::Ptr resource, ResourceState state) = 0;
};
