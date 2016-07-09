#pragma once

#include <platform.h>
#include <string>
#include <utilities.h>

struct ShaderLight
{
    ShaderLight()
    {
        std::string path = PROJECT_RESOURCE_DIR "../samples/Demo/shaders/";

        std::ifstream vertex_stream(path + "ShaderLight.vs");
        std::string vertex((std::istreambuf_iterator<char>(vertex_stream)), std::istreambuf_iterator<char>());

        std::ifstream fragment_stream(path + "ShaderLight.fs");
        std::string fragment((std::istreambuf_iterator<char>(fragment_stream)), std::istreambuf_iterator<char>());

        program = createProgram(vertex.c_str(), fragment.c_str());

        loc_viewPos = glGetUniformLocation(program, "viewPos");
        loc_lightPos = glGetUniformLocation(program, "lightPos");

        loc_model = glGetUniformLocation(program, "model");
        loc_view = glGetUniformLocation(program, "view");
        loc_projection = glGetUniformLocation(program, "projection");
        loc_depthBiasMVP = glGetUniformLocation(program, "depthBiasMVP");

        loc_material.ambient = glGetUniformLocation(program, "material.ambient");
        loc_material.diffuse = glGetUniformLocation(program, "material.diffuse");
        loc_material.specular = glGetUniformLocation(program, "material.specular");
        loc_material.shininess = glGetUniformLocation(program, "material.shininess");

        loc_light.ambient = glGetUniformLocation(program, "light.ambient");
        loc_light.diffuse = glGetUniformLocation(program, "light.diffuse");
        loc_light.specular = glGetUniformLocation(program, "light.specular");
    }

    struct LocMaterial
    {
        GLuint ambient;
        GLuint diffuse;
        GLuint specular;
        GLuint shininess;
    } loc_material;

    struct LocLight
    {
        GLuint ambient;
        GLuint diffuse;
        GLuint specular;
    } loc_light;

    GLuint program;

    GLint loc_viewPos;
    GLint loc_lightPos;

    GLint loc_model;
    GLint loc_view;
    GLint loc_projection;
    GLint loc_depthBiasMVP;
};

struct ShaderSimpleColor
{
    ShaderSimpleColor()
    {
        std::string path = PROJECT_RESOURCE_DIR "../samples/Demo/shaders/";

        std::ifstream vertex_stream(path + "ShaderSimpleColor.vs");
        std::string vertex((std::istreambuf_iterator<char>(vertex_stream)), std::istreambuf_iterator<char>());

        std::ifstream fragment_stream(path + "ShaderSimpleColor.fs");
        std::string fragment((std::istreambuf_iterator<char>(fragment_stream)), std::istreambuf_iterator<char>());

        program = createProgram(vertex.c_str(), fragment.c_str());

        loc_objectColor = glGetUniformLocation(program, "objectColor");
        loc_uMVP = glGetUniformLocation(program, "u_MVP");
    }

    GLuint program;
    GLint loc_objectColor;
    GLint loc_uMVP;
};

struct ShaderShadowView
{
    ShaderShadowView()
    {
        std::string path = PROJECT_RESOURCE_DIR "../samples/Demo/shaders/";

        std::ifstream vertex_stream(path + "ShaderShadowView.vs");
        std::string vertex((std::istreambuf_iterator<char>(vertex_stream)), std::istreambuf_iterator<char>());

        std::ifstream fragment_stream(path + "ShaderShadowView.fs");
        std::string fragment((std::istreambuf_iterator<char>(fragment_stream)), std::istreambuf_iterator<char>());

        program = createProgram(vertex.c_str(), fragment.c_str());
        loc_MVP = glGetUniformLocation(program, "u_m4MVP");
    }

    GLuint program;
    GLint loc_MVP;
};

struct ShaderDepth
{
    ShaderDepth()
    {
        std::string path = PROJECT_RESOURCE_DIR "../samples/Demo/shaders/";

        std::ifstream vertex_stream(path + "ShaderDepth.vs");
        std::string vertex((std::istreambuf_iterator<char>(vertex_stream)), std::istreambuf_iterator<char>());

        std::ifstream fragment_stream(path + "ShaderDepth.fs");
        std::string fragment((std::istreambuf_iterator<char>(fragment_stream)), std::istreambuf_iterator<char>());

        program = createProgram(vertex.c_str(), fragment.c_str());
        loc_MVP = glGetUniformLocation(program, "u_m4MVP");
    }

    GLuint program;
    GLint loc_MVP;
};