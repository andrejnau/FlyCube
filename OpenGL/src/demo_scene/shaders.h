#pragma once

#include <platform.h>
#include <string>
#include <utilities.h>

#define GET_UNIFORM_LOCATION(name) \
    loc.name = glGetUniformLocation(program, #name);

struct ShaderSSAOBlurPass
{
    ShaderSSAOBlurPass()
    {
        std::string vertex = GetShaderSource("shaders/Demo/SSAOPass.vs");
        std::string fragment = GetShaderSource("shaders/Demo/SSAOBlurPass.fs");

        ShaderVector shaders = {
            { GL_VERTEX_SHADER, vertex },
            { GL_FRAGMENT_SHADER, fragment }
        };

        program = CreateProgram(shaders);

        GET_UNIFORM_LOCATION(ssaoInput);
    }

    GLuint program;

    struct
    {
        GLuint ssaoInput;
    } loc;
};

struct ShaderSSAOPass
{
    ShaderSSAOPass()
    {
        std::string vertex = GetShaderSource("shaders/Demo/SSAOPass.vs");
        std::string fragment = GetShaderSource("shaders/Demo/SSAOPass.fs");

        ShaderVector shaders = {
            { GL_VERTEX_SHADER, vertex },
            { GL_FRAGMENT_SHADER, fragment }
        };

        program = CreateProgram(shaders);

        GET_UNIFORM_LOCATION(gPosition);
        GET_UNIFORM_LOCATION(gNormal);
        GET_UNIFORM_LOCATION(samples);
        GET_UNIFORM_LOCATION(projection);
        GET_UNIFORM_LOCATION(noiseTexture);
    }

    GLuint program;

    struct
    {
        GLuint gPosition;
        GLuint gNormal;
        GLuint samples;
        GLuint projection;
        GLuint noiseTexture;
    } loc;
};

struct ShaderLightPass
{
    ShaderLightPass()
    {
        std::string vertex = GetShaderSource("shaders/Demo/LightPass.vs");
        std::string fragment = GetShaderSource("shaders/Demo/LightPass.fs");

        ShaderVector shaders = {
            { GL_VERTEX_SHADER, vertex },
            { GL_FRAGMENT_SHADER, fragment }
        };

        program = CreateProgram(shaders);

        GET_UNIFORM_LOCATION(gPosition);
        GET_UNIFORM_LOCATION(gNormal);
        GET_UNIFORM_LOCATION(gAmbient);
        GET_UNIFORM_LOCATION(gDiffuse);
        GET_UNIFORM_LOCATION(gSpecular);
        GET_UNIFORM_LOCATION(gSSAO);
        GET_UNIFORM_LOCATION(has_SSAO);

        GET_UNIFORM_LOCATION(viewPos);
        GET_UNIFORM_LOCATION(lightPos);

        GET_UNIFORM_LOCATION(depthBiasMVP);
        GET_UNIFORM_LOCATION(depthTexture);
        GET_UNIFORM_LOCATION(has_depthTexture);
    }

    GLuint program;

    struct
    {
        GLuint gPosition;
        GLuint gNormal;
        GLuint gAmbient;
        GLuint gDiffuse;
        GLuint gSpecular;
        GLuint gSSAO;

        GLint viewPos;
        GLint lightPos;
        GLint depthBiasMVP;

        GLint depthTexture;
        GLint has_depthTexture;
        GLint has_SSAO;
    } loc;
};

struct ShaderGeometryPass
{
    ShaderGeometryPass()
    {
        std::string vertex = GetShaderSource("shaders/Demo/GeometryPass.vs");
        std::string fragment = GetShaderSource("shaders/Demo/GeometryPass.fs");

        ShaderVector shaders = {
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

        GET_UNIFORM_LOCATION(model);
        GET_UNIFORM_LOCATION(view);
        GET_UNIFORM_LOCATION(projection);

        GET_UNIFORM_LOCATION(textures.normalMap);
        GET_UNIFORM_LOCATION(textures.has_normalMap);

        GET_UNIFORM_LOCATION(textures.ambient);
        GET_UNIFORM_LOCATION(textures.has_ambient);

        GET_UNIFORM_LOCATION(textures.diffuse);
        GET_UNIFORM_LOCATION(textures.has_diffuse);

        GET_UNIFORM_LOCATION(textures.specular);
        GET_UNIFORM_LOCATION(textures.has_specular);

        GET_UNIFORM_LOCATION(textures.alpha);
        GET_UNIFORM_LOCATION(textures.has_alpha);
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

        struct Texture
        {
            GLuint normalMap;
            GLuint has_normalMap;

            GLuint ambient;
            GLuint has_ambient;

            GLuint diffuse;
            GLuint has_diffuse;

            GLuint specular;
            GLuint has_specular;

            GLuint alpha;
            GLuint has_alpha;
        } textures;

        GLint model;
        GLint view;
        GLint projection;
    } loc;
};

struct ShaderSimpleColor
{
    ShaderSimpleColor()
    {
        std::string vertex = GetShaderSource("shaders/Demo/ShaderSimpleColor.vs");
        std::string fragment = GetShaderSource("shaders/Demo/ShaderSimpleColor.fs");

        ShaderVector shaders = {
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

        ShaderVector shaders = {
            { GL_VERTEX_SHADER, vertex },
            { GL_FRAGMENT_SHADER, fragment }
        };

        program = CreateProgram(shaders);

        GET_UNIFORM_LOCATION(u_m4MVP);
        GET_UNIFORM_LOCATION(sampler);
    }

    GLuint program;

    struct
    {
        GLint u_m4MVP;
        GLint sampler;
    } loc;
};

struct ShaderDepth
{
    ShaderDepth()
    {
        std::string vertex = GetShaderSource("shaders/Demo/ShaderDepth.vs");
        std::string fragment = GetShaderSource("shaders/Demo/ShaderDepth.fs");

        ShaderVector shaders = {
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

        ShaderVector shaders = {
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