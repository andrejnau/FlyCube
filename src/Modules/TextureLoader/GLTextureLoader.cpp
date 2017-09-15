#include "TextureLoader/GLTextureLoader.h"
#include <gli/gli.hpp>
#include <map>

GLuint CreateTexture(TextureInfo& texture_info)
{
    static std::map<std::string, GLuint> cache;
    auto it = cache.find(texture_info.path);
    if (it != cache.end())
        return it->second;

    gli::texture texture = gli::load(texture_info.path);
    if (texture.empty())
        return 0;

    gli::gl GL(gli::gl::PROFILE_GL33);
    gli::gl::format const format = GL.translate(texture.format(), texture.swizzles());
    GLenum target = GL.translate(texture.target());
    assert(texture.target() == gli::TARGET_2D);

    GLuint texture_name = 0;
    glGenTextures(1, &texture_name);
    glBindTexture(target, texture_name);
    glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(texture.levels() - 1));
    glTexParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, &format.Swizzles[0]);
    glTexStorage2D(target, static_cast<GLint>(texture.levels()), format.Internal, texture.extent(0).x, texture.extent(0).y);
    bool is_compressed = gli::is_compressed(texture.format());
    for (std::size_t level = 0; level < texture.levels(); ++level)
    {
        glm::tvec3<GLsizei> extent(texture.extent(level));
        if (is_compressed)
        {
            glCompressedTexSubImage2D(target, static_cast<GLint>(level), 0, 0, extent.x, extent.y,
                format.Internal, static_cast<GLsizei>(texture.size(level)), texture.data(0, 0, level));
        }
        else
        {
            glTexSubImage2D(target, static_cast<GLint>(level), 0, 0, extent.x, extent.y, format.External, format.Type, texture.data(0, 0, level));
        }
    }

    if (texture_info.type == aiTextureType_OPACITY)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)GL_NEAREST);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)GL_LINEAR);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)GL_REPEAT);
    glBindTexture(target, 0);

    return cache[texture_info.path] = texture_name;
}
