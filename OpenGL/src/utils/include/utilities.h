#pragma once

#include <platform.h>
#include <string>
#include <vector>
#include <fstream>

#define POS_ATTRIB 0
#define NORMAL_ATTRIB 1
#define TEXTURE_ATTRIB 2
#define TANGENT_ATTRIB 3
#define BITANGENT_ATTRIB 4

using ShaderVector = std::vector<std::pair<GLenum, std::string>>;

inline std::string GetAssetFullPath(const std::string &assetName)
{
    return std::string(ASSETS_PATH) + assetName;
}

inline std::string GetShaderSource(const std::string &file)
{
    std::ifstream stream(GetAssetFullPath(file));
    std::string shader((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    return shader;
}

inline GLuint CreateShader(GLenum shaderType, const std::string &src)
{
    GLuint shader = glCreateShader(shaderType);
    const GLchar *source[] = { src.c_str() };
    glShaderSource(shader, 1, source, nullptr);
    glCompileShader(shader);
    return shader;
}

inline GLuint CreateProgram(const ShaderVector &shaders)
{
    std::vector<GLuint> shadersId;
    for (auto & shader : shaders)
    {
        GLuint id = CreateShader(shader.first, shader.second.c_str());
        shadersId.push_back(id);
    }
    GLuint program = glCreateProgram();
    for (auto & id : shadersId)
    {
        glAttachShader(program, id);
    }
    glLinkProgram(program);
    return program;
}