#include "TextureCache.h"

TextureCache::TextureCache(Context& context)
    : m_context(context)
{
}

Resource::Ptr TextureCache::Load(const std::string& path)
{
    auto it = m_cache.find(path);
    if (it == m_cache.end())
        it = m_cache.emplace(path, CreateTexture(m_context, path)).first;
    return it->second;
}
