﻿#pragma once

#include <platform.h>
#include <string>
#include <utilities.h>

struct ShaderLight
{
    ShaderLight()
    {
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

    std::string vertex =
        R"vs(
            #version 330 core
            precision highp float;
            layout(location = )vs" STRV(POS_ATTRIB) R"vs() in vec3 position;
            layout(location = )vs" STRV(NORMAL_ATTRIB) R"vs() in vec3 normal;
            layout(location = )vs" STRV(TEXTURE_ATTRIB) R"vs() in vec2 texCoords;
            layout(location = )vs" STRV(TANGENT_ATTRIB) R"vs() in vec3 tangent;
            layout(location = )vs" STRV(BITANGENT_ATTRIB) R"vs() in vec3 bitangent;

            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            uniform mat4 depthBiasMVP;

            uniform vec3 lightPos;
            uniform vec3 viewPos;

            out VS_OUT
            {
                vec3 FragPos;
                vec3 Normal;
                vec2 TexCoords;
                vec3 Tangent;
                vec3 Bitangent;
                vec3 LightPos;
                vec3 ViewPos;
                vec4 ShadowCoord;
            } vs_out;

            void main()
            {
                gl_Position = projection * view * model * vec4(position, 1.0f);
                vs_out.FragPos = vec3(model * vec4(position, 1.0f));
                vs_out.TexCoords = texCoords;
                vs_out.LightPos = lightPos;
                vs_out.ViewPos  = viewPos;

                mat3 normalMatrix = mat3(transpose(inverse(model)));
                vec3 T = normalize(normalMatrix * tangent);
                vec3 N = normalize(normalMatrix * normal);
                T = normalize(T - dot(T, N) * N);
                vec3 B = cross(T, N);

                vs_out.Tangent = T;
                vs_out.Bitangent = B;
                vs_out.Normal = N;

                vs_out.ShadowCoord = depthBiasMVP * model * vec4(position, 1.0f);
            }
        )vs";

        std::string fragment =
            R"fs(
            #version 330 core
            precision highp float;
            out vec4 out_Color;

            struct Material
            {
                vec3 ambient;
                vec3 diffuse;
                vec3 specular;
                float shininess;
            };

            struct Light
            {
                vec3 ambient;
                vec3 diffuse;
                vec3 specular;
            };

            struct Texture
            {
                sampler2D normalMap;
                int has_normalMap;

                sampler2D ambient;
                int has_ambient;

                sampler2D diffuse;
                int has_diffuse;

                sampler2D specular;
                int has_specular;

                sampler2D alpha;
                int has_alpha;
            };

            uniform sampler2DShadow depthTexture;
            uniform int has_depthTexture;

            uniform Material material;
            uniform Light light;
            uniform Texture textures;

            in VS_OUT
            {
                vec3 FragPos;
                vec3 Normal;
                vec2 TexCoords;
                vec3 Tangent;
                vec3 Bitangent;
                vec3 LightPos;
                vec3 ViewPos;
                vec4 ShadowCoord;
            } fs_in;

            vec3 CalcBumpedNormal()
            {
                // Obtain normal from normal map in range [0,1]
                vec3 normal = texture(textures.normalMap, fs_in.TexCoords).rgb;
                // Transform normal vector to range [-1,1]
                normal = normalize(normal * 2.0 - 1.0);
                // Then transform normal in tangent space to world-space via TBN matrix
                mat3 tbn = mat3(fs_in.Tangent, fs_in.Bitangent, fs_in.Normal); // TBN calculated in fragment shader
                normal = normalize(tbn * normal);
                return normal;
            }

            float GetShadowPCF(in vec4 smcoord)
            {
                float res = 0.0;
                smcoord.z -= 0.0005 * smcoord.w;
                res += textureProjOffset(depthTexture, smcoord, ivec2(-1, -1));
                res += textureProjOffset(depthTexture, smcoord, ivec2(0, -1));
                res += textureProjOffset(depthTexture, smcoord, ivec2(1, -1));
                res += textureProjOffset(depthTexture, smcoord, ivec2(-1, 0));
                res += textureProjOffset(depthTexture, smcoord, ivec2(0, 0));
                res += textureProjOffset(depthTexture, smcoord, ivec2(1, 0));
                res += textureProjOffset(depthTexture, smcoord, ivec2(-1, 1));
                res += textureProjOffset(depthTexture, smcoord, ivec2(0, 1));
                res += textureProjOffset(depthTexture, smcoord, ivec2(1, 1));
                res /= 9.0;
                return max(0.25, res);
            }

            void main()
            {
                vec3 normal = fs_in.Normal;
                if (textures.has_normalMap != 0)
                    normal = CalcBumpedNormal();

                // Ambient
                vec3 ambient = light.ambient * material.ambient;
                if (textures.has_ambient != 0)
                    ambient *= texture(textures.ambient, fs_in.TexCoords).rgb;

                // Diffuse
                vec3 lightDir = normalize(fs_in.LightPos - fs_in.FragPos);
                float diff = max(dot(lightDir, normal), 0.0);
                vec3 diffuse = light.diffuse * diff * material.diffuse;
                if (textures.has_diffuse != 0)
                    diffuse *= texture(textures.diffuse, fs_in.TexCoords).rgb;

                // Specular
                vec3 viewDir = normalize(fs_in.ViewPos - fs_in.FragPos);
                vec3 reflectDir = reflect(-lightDir, normal);
                float spec = pow(max(dot(normal, reflectDir), 0.0), material.shininess);
                vec3 specular = light.specular * spec * material.specular;
                if (textures.has_specular != 0)
                    specular *= texture(textures.specular, fs_in.TexCoords).rgb;

                float shadow  = 1.0;
                if (has_depthTexture != 0)
                {
                    shadow = GetShadowPCF(fs_in.ShadowCoord);
                }

                vec4 result = vec4(ambient + diffuse * shadow + specular * shadow, 1.0);

                if (textures.has_alpha != 0)
                {
                    result.a = texture(textures.alpha, fs_in.TexCoords).r;
                    if (result.a < 0.5)
                        discard;
                }

                out_Color = result;
            }
        )fs";
};

struct ShaderSimpleColor
{
    ShaderSimpleColor()
    {
        program = createProgram(vertex.c_str(), fragment.c_str());

        loc_objectColor = glGetUniformLocation(program, "objectColor");
        loc_uMVP = glGetUniformLocation(program, "u_MVP");
    }

    GLuint program;
    GLint loc_objectColor;
    GLint loc_uMVP;

    std::string vertex =
        R"vs(
            #version 300 es
            precision highp float;
            layout(location = )vs" STRV(POS_ATTRIB) R"vs() in vec3 a_pos;

                    uniform mat4 u_MVP;

                    void main()
            {
                gl_Position = u_MVP * vec4(a_pos, 1.0);
            }
        )vs";

    std::string fragment =
        R"fs(
            #version 300 es
            precision highp float;
            out vec4 out_Color;

                    uniform vec3 objectColor;

                    void main()
            {
                out_Color = vec4(objectColor, 1.0);
            }
        )fs";
};

struct ShaderShadowView
{
    ShaderShadowView()
    {
        program = createProgram(vertex.c_str(), fragment.c_str());
        loc_MVP = glGetUniformLocation(program, "u_m4MVP");
    }

    GLuint program;
    GLint loc_MVP;

    std::string vertex =
        "#version 300 es\n"
        "precision mediump float;\n"
        "layout(location = " STRV(POS_ATTRIB) ") in vec3 pos;\n"
        "layout(location = " STRV(TEXTURE_ATTRIB) ") in vec2 texCoord;\n"
        "uniform mat4 u_m4MVP;\n"
        "out vec2 texCoordFS;\n"
        "void main() {\n"
        "    texCoordFS = texCoord;\n"
        "    gl_Position = u_m4MVP * vec4(pos, 1.0);\n"
        "}\n";

    std::string fragment =
        "#version 300 es\n"
        "precision mediump float;\n"
        "uniform sampler2D sampler;\n"
        "in vec2 texCoordFS;\n"
        "out vec4 outColor;\n"
        "void main() {\n"
        "    vec4 color = texture(sampler, texCoordFS);\n"
        "    outColor = color;\n"
        "}\n";
};

struct ShaderDepth
{
    ShaderDepth()
    {
        program = createProgram(vertex.c_str(), fragment.c_str());
        loc_MVP = glGetUniformLocation(program, "u_m4MVP");
    }

    GLuint program;
    GLint loc_MVP;

    std::string vertex =
        "#version 300 es\n"
        "precision mediump float;\n"
        "layout(location = " STRV(POS_ATTRIB) ") in vec3 pos;\n"
        "uniform mat4 u_m4MVP;\n"
        "void main() {\n"
        "    gl_Position = u_m4MVP * vec4(pos, 1.0);\n"
        "}\n";

    std::string fragment =
        "#version 300 es\n"
        "void main() {\n"
        "}\n";
};