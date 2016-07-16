#pragma once

#include <platform.h>
#include <string>
#include <utilities.h>

#define GET_UNIFORM_LOCATION(name) \
    loc.##name = glGetUniformLocation(program, #name);

struct ShaderLight
{
    ShaderLight()
    {
        std::string vertex = GetShaderSource("shaders/Demo/ShaderLight.vs");
        std::string fragment = GetShaderSource("shaders/Demo/ShaderLight.fs");

        ShadersVector shaders = {
            { GL_VERTEX_SHADER, vertex },
            { GL_FRAGMENT_SHADER, fragment }
        };

        program = CreateProgram(shaders);

        GET_UNIFORM_LOCATION(material.ambient);
        GET_UNIFORM_LOCATION(material.diffuse);
        GET_UNIFORM_LOCATION(material.specular);
        GET_UNIFORM_LOCATION(material.shininess);

        GET_UNIFORM_LOCATION(light.ambient);
        GET_UNIFORM_LOCATION(light.diffuse);
        GET_UNIFORM_LOCATION(light.specular);

        GET_UNIFORM_LOCATION(viewPos);
        GET_UNIFORM_LOCATION(lightPos);

        GET_UNIFORM_LOCATION(model);
        GET_UNIFORM_LOCATION(view);
        GET_UNIFORM_LOCATION(projection);
        GET_UNIFORM_LOCATION(depthBiasMVP);
    }

    GLuint program;

    struct
    {
        struct
        {
            GLuint ambient;
            GLuint diffuse;
            GLuint specular;
            GLuint shininess;
        } material;

        struct
        {
            GLuint ambient;
            GLuint diffuse;
            GLuint specular;
        } light;


        GLint viewPos;
        GLint lightPos;

        GLint model;
        GLint view;
        GLint projection;
        GLint depthBiasMVP;
    } loc;
};

struct ShaderSimpleColor
{
    ShaderSimpleColor()
    {
        std::string vertex = GetShaderSource("shaders/Demo/ShaderSimpleColor.vs");
        std::string fragment = GetShaderSource("shaders/Demo/ShaderSimpleColor.fs");

        ShadersVector shaders = {
            { GL_VERTEX_SHADER, vertex },
            { GL_FRAGMENT_SHADER, fragment }
        };

        program = CreateProgram(shaders);

        GET_UNIFORM_LOCATION(objectColor);
        GET_UNIFORM_LOCATION(u_MVP);
    }

    GLuint program;

    struct
    {
        GLint objectColor;
        GLint u_MVP;
    } loc;
};

struct ShaderShadowView
{
    ShaderShadowView()
    {
        std::string vertex = GetShaderSource("shaders/Demo/ShaderShadowView.vs");
        std::string fragment = GetShaderSource("shaders/Demo/ShaderShadowView.fs");

        ShadersVector shaders = {
            { GL_VERTEX_SHADER, vertex },
            { GL_FRAGMENT_SHADER, fragment }
        };

        program = CreateProgram(shaders);

        GET_UNIFORM_LOCATION(u_m4MVP);
    }

    GLuint program;

    struct
    {
        GLint u_m4MVP;
    } loc;
};

struct ShaderDepth
{
    ShaderDepth()
    {
        std::string vertex = GetShaderSource("shaders/Demo/ShaderDepth.vs");
        std::string fragment = GetShaderSource("shaders/Demo/ShaderDepth.fs");

        ShadersVector shaders = {
            { GL_VERTEX_SHADER, vertex },
            { GL_FRAGMENT_SHADER, fragment }
        };

        program = CreateProgram(shaders);

        GET_UNIFORM_LOCATION(u_m4MVP);
    }

    GLuint program;

    struct
    {
        GLint u_m4MVP;
    } loc;
};

struct ShaderSimpleCubeMap
{
    ShaderSimpleCubeMap()
    {
        std::string vertex = GetShaderSource("shaders/Demo/ShaderSimpleCubeMap.vs");
        std::string fragment = GetShaderSource("shaders/Demo/ShaderSimpleCubeMap.fs");

        ShadersVector shaders = {
            { GL_VERTEX_SHADER, vertex },
            { GL_FRAGMENT_SHADER, fragment }
        };

        program = CreateProgram(shaders);

        GET_UNIFORM_LOCATION(u_MVP);
    }

    GLuint program;

    struct
    {
        GLint u_MVP;
    } loc;
};