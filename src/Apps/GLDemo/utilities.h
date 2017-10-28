#pragma once

#include <glLoadGen/gl.h>
#include <string>
#include <vector>
#include <fstream>

#define POS_ATTRIB 0
#define NORMAL_ATTRIB 1
#define TANGENT_ATTRIB 2
#define TEXTURE_ATTRIB 3

using ShaderVector = std::vector<std::pair<GLenum, std::string>>;

inline std::string GetAssetFullPath(const std::string &assetName)
{
    std::string path = std::string(ASSETS_PATH) + assetName;
    if (!std::ifstream(path).good())
        path = "assets/" + assetName;
    return path;
}

inline std::string GetShaderSource(const std::string &file)
{
    std::ifstream stream(GetAssetFullPath(file));
    std::string shader((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    return shader;
}

inline void CheckLink(GLuint program)
{
    std::string err;
    GLint link_ok;
    glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
    if (!link_ok)
    {
        GLint infoLogLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
        std::vector<GLchar> info_log(infoLogLength);
        glGetProgramInfoLog(program, info_log.size(), nullptr, info_log.data());
        err = info_log.data();
    }
}

inline void CheckCompile(GLuint shader)
{
    std::string err;
    GLint link_ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &link_ok);
    if (!link_ok)
    {
        GLint infoLogLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
        std::vector<GLchar> info_log(infoLogLength);
        glGetShaderInfoLog(shader, info_log.size(), nullptr, info_log.data());
        err = info_log.data();
    }
}

inline GLuint CreateShader(GLenum shaderType, const std::string &src)
{
    GLuint shader = glCreateShader(shaderType);
    const GLchar *source[] = { src.c_str() };
    glShaderSource(shader, 1, source, nullptr);
    glCompileShader(shader);
    CheckCompile(shader);
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
    CheckLink(program);
    return program;
}