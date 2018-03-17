#pragma once

#include <wrl.h>
#include <memory>
#include <string>
#include <vector>

#include <Utilities/FileUtility.h>

using namespace Microsoft::WRL;

class Resource
{
public:
    virtual ~Resource() = default;
    virtual void SetName(const std::string& name) = 0;
    using Ptr = std::shared_ptr<Resource>;
};
