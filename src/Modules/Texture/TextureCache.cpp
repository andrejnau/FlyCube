#include "Texture/TextureCache.h"
#include <Utilities/FormatHelper.h>

TextureCache::TextureCache(Context& context)
    : m_context(context)
{
}

std::shared_ptr<Resource> TextureCache::Load(const std::string& path)
{
    auto it = m_cache.find(path);
    if (it == m_cache.end())
        it = m_cache.emplace(path, CreateTexture(m_context, path)).first;
    return it->second;
}

std::shared_ptr<Resource> TextureCache::CreateTextuteStab(const glm::vec4& val)
{
    auto it = m_stub_cache.find(val);
    if (it != m_stub_cache.end())
        return it->second;
    std::shared_ptr<Resource> tex = m_context.CreateTexture(BindFlag::kSrv, gli::format::FORMAT_RGBA32_SFLOAT_PACK32, 1, 1, 1, 1);
    size_t num_bytes = 0;
    size_t row_bytes = 0;
    GetFormatInfo(1, 1, gli::format::FORMAT_RGBA32_SFLOAT_PACK32, num_bytes, row_bytes);
    m_context.UpdateSubresource(tex, 0, &val, row_bytes, num_bytes);
    m_stub_cache.emplace(val, tex);
    return tex;
}
