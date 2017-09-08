#pragma once

#include <Geometry/Geometry.h>
#include <glm/glm.hpp>
#include <gli/gli.hpp>
#include <utilities.h>

inline GLuint CreateCompressedTexture(const std::string& Filename, aiTextureType type)
{
    gli::texture Texture = gli::load(Filename);
    if (Texture.empty())
        return 0;

    gli::gl GL(gli::gl::PROFILE_GL33);
    gli::gl::format const Format = GL.translate(Texture.format(), Texture.swizzles());
    GLenum Target = GL.translate(Texture.target());
    //assert(gli::is_compressed(Texture.format()) && Target == gli::TARGET_2D);

    GLuint TextureName = 0;
    glGenTextures(1, &TextureName);
    glBindTexture(Target, TextureName);
    glTexParameteri(Target, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(Target, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(Texture.levels() - 1));
    glTexParameteriv(Target, GL_TEXTURE_SWIZZLE_RGBA, &Format.Swizzles[0]);
    glTexStorage2D(Target, static_cast<GLint>(Texture.levels()), Format.Internal, Texture.extent(0).x, Texture.extent(0).y);
    for (std::size_t Level = 0; Level < Texture.levels(); ++Level)
    {
        glm::tvec3<GLsizei> Extent(Texture.extent(Level));
        glCompressedTexSubImage2D(Target, static_cast<GLint>(Level), 0, 0, Extent.x, Extent.y,
            Format.Internal, static_cast<GLsizei>(Texture.size(Level)), Texture.data(0, 0, Level));
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
    glBindTexture(Target, 0);

    return TextureName;
}

inline GLuint TextureFromFile(IMesh::Texture& texture)
{
    DBG("texture %s", texture.path.c_str());
    return CreateCompressedTexture(texture.path, texture.type);
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

        glEnableVertexAttribArray(BITANGENT_ATTRIB);
        glVertexAttribPointer(BITANGENT_ATTRIB, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, bitangent));
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
        glDisableVertexAttribArray(BITANGENT_ATTRIB);
    }
};
