#pragma once

#include <Geometry/Geometry.h>
#include <glm/glm.hpp>
#include <gli/gli.hpp>
#include <utilities.h>
#include <Utilities/Logger.h>

inline GLuint CreateTexture(const std::string& filename, aiTextureType type)
{
    static std::map<std::string, GLuint> cache;
    auto it = cache.find(filename);
    if (it != cache.end())
        return it->second;

    gli::texture texture = gli::load(filename);
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

    if (type == aiTextureType_OPACITY)
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

    return cache[filename] = texture_name;
}

inline GLuint TextureFromFile(TextureInfo& texture)
{
    DBG("texture %s", texture.path.c_str());
    return CreateTexture(texture.path, texture.type);
}

struct GLMesh : IMesh
{
    GLuint VAO, VBO, EBO;

    std::vector<GLuint> textures_id;

    void setupMesh()
    {
        for (auto & tex : textures)
        {
            textures_id.push_back(TextureFromFile(tex));
        }

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glBindVertexArray(0);
    }

    void drawMesh()
    {
        bindMesh();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
        unbindMesh();
    }

    void bindMesh()
    {
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

        // Vertex Positions
        glEnableVertexAttribArray(POS_ATTRIB);
        glVertexAttribPointer(POS_ATTRIB, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, position));
        // Vertex Normals
        glEnableVertexAttribArray(NORMAL_ATTRIB);
        glVertexAttribPointer(NORMAL_ATTRIB, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, normal));
        // Vertex Texture Coords
        glEnableVertexAttribArray(TEXTURE_ATTRIB);
        glVertexAttribPointer(TEXTURE_ATTRIB, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, texCoords));

        glEnableVertexAttribArray(TANGENT_ATTRIB);
        glVertexAttribPointer(TANGENT_ATTRIB, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, tangent));
    }

    void unbindMesh()
    {
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDisableVertexAttribArray(POS_ATTRIB);
        glDisableVertexAttribArray(NORMAL_ATTRIB);
        glDisableVertexAttribArray(TEXTURE_ATTRIB);
        glDisableVertexAttribArray(TANGENT_ATTRIB);
    }
};
