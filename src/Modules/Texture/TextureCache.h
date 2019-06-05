#pragma once

#include "TextureLoader.h"
#include <glm/glm.hpp>

class TextureCache
{
public:
    TextureCache(Context& context);
    Resource::Ptr Load(const std::string& path);
    Resource::Ptr CreateTextuteStab(const glm::vec4& val);

private:
    Context& m_context;
    std::map<std::string, Resource::Ptr> m_cache;

    struct glm_key
    {
        glm_key(glm::vec4 val) : val(val) {}
        bool operator<(const glm_key& oth) const
        {
            for (size_t i = 0; i < val.length(); ++i)
            {
                if (val[i] == oth.val[i])
                    continue;
                return val[i] < oth.val[i];
            }
            return false;
        }
        glm::vec4 val;
    };

    std::map<glm_key, Resource::Ptr> m_stub_cache;
};
