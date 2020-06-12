#pragma once
#include "TextureLoader.h"
#include <glm/glm.hpp>

class TextureCache
{
public:
    TextureCache(Device& device, CommandListBox& command_list);
    std::shared_ptr<Resource> Load(const std::string& path);
    std::shared_ptr<Resource> CreateTextuteStab(const glm::vec4& val);

private:
    Device& m_device;
    CommandListBox& m_command_list;
    std::map<std::string, std::shared_ptr<Resource>> m_cache;

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

    std::map<glm_key, std::shared_ptr<Resource>> m_stub_cache;
};
