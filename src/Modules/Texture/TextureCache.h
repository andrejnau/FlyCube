#pragma once

#include "TextureLoader.h"

class TextureCache
{
public:
    TextureCache(Context& context);
    Resource::Ptr Load(const std::string& path);

private:
    Context& m_context;
    std::map<std::string, Resource::Ptr> m_cache;
};
