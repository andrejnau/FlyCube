#pragma once
#include <Resource/Resource.h>
#include <View/View.h>
#include <Instance/BaseTypes.h>
#include <memory>
#include <array>

class CommandList
{
public:
    virtual ~CommandList() = default;
    virtual void Open() = 0;
    virtual void Close() = 0;
    virtual void Clear(const std::shared_ptr<View>& view, const std::array<float, 4>& color) = 0;
    virtual void ResourceBarrier(const std::shared_ptr<Resource>& resource, ResourceState state) = 0;
};
