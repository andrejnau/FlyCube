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

using ShadersVector = std::vector<std::pair<GLenum, std::string>>;

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

inline const char* ShaderType2Mgs(GLenum shaderType)
{
    switch (shaderType)
    {
    case GL_VERTEX_SHADER:
        return "vertex";
    case GL_FRAGMENT_SHADER:
        return "fragment";
    case GL_GEOMETRY_SHADER:
        return "geometry";
    default:
        return "undefined shader type";
    }
}

inline GLuint CreateShader(GLenum shaderType, const char *src)
{
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (static_cast<GLboolean>(compiled) == GL_FALSE)
    {
        GLint infoLogLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);
        if (infoLogLen > 0)
        {
            std::vector<GLchar> infoLog(infoLogLen);
            glGetShaderInfoLog(shader, infoLogLen, nullptr, infoLog.data());
            DBG("Could not compile %s shader:\n%s\n", ShaderType2Mgs(shaderType), infoLog.data());
        }
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

inline GLuint CreateProgram(const ShadersVector &shaders)
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

    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (static_cast<GLboolean>(linked) == GL_FALSE)
    {
        DBG("Could not link program");
        GLint infoLogLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLen);
        if (infoLogLen > 0)
        {
            std::vector<GLchar> infoLog(infoLogLen);
            glGetProgramInfoLog(program, infoLogLen, nullptr, infoLog.data());
            DBG("Could not link program:\n%s\n", infoLog.data());
        }
        glDeleteProgram(program);
        return 0;
    }

    return program;
}