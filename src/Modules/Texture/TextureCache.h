#pragma once

#include "TextureLoader.h"
#include "DXGIFormatHelper.h"
#include <glm/glm.hpp>

class TextureCache
{
public:
    TextureCache(Context& context);
    Resource::Ptr Load(const std::string& path);

    Resource::Ptr CreateTextuteStab(glm::vec4& val)
    {
        Resource::Ptr tex = m_context.CreateTexture(BindFlag::kSrv, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 1, 1, 1);
        size_t num_bytes = 0;
        size_t row_bytes = 0;
        GetSurfaceInfo(1, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, &num_bytes, &row_bytes, nullptr);
        m_context.UpdateSubresource(tex, 0, &val, row_bytes, num_bytes);
        return tex;
    }

private:
    Context& m_context;
    std::map<std::string, Resource::Ptr> m_cache;
};
