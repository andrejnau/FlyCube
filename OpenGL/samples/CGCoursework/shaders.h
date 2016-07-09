#pragma once

#include <platform.h>
#include <string>
#include <utilities.h>

struct ShaderSimpleCubeMap
{
    ShaderSimpleCubeMap()
    {
        program = createProgram(vertex.c_str(), fragment.c_str());
        loc_MVP = glGetUniformLocation(program, "u_MVP");
    }

    GLuint program;
    GLint loc_MVP;

    std::string vertex =
        "#version 300 es\n"
        "precision mediump float;\n"
        "layout(location = " STRV(POS_ATTRIB) ") in vec3 a_pos;\n"
        "uniform mat4 u_MVP;\n"
        "out vec3 texCoord;\n"
        "void main() {\n"
        "    gl_Position = u_MVP * vec4(a_pos, 1.0);\n"
        "    texCoord = normalize(a_pos);\n"
        "}\n";

    std::string fragment =
        "#version 300 es\n"
        "precision mediump float;\n"
        "uniform samplerCube cubemap;\n"
        "in vec3 texCoord;\n"
        "out vec4 outColor;\n"
        "void main()\n"
        "{\n"
        "    outColor = texture(cubemap, texCoord);\n"
        "}\n";
};

struct ShaderTexture
{
    ShaderTexture()
    {
        program = createProgram(vertex.c_str(), fragment.c_str());
        loc_MVP = glGetUniformLocation(program, "u_m4MVP");
        loc_sampler = glGetUniformLocation(program, "sampler");
    }

    GLuint program;
    GLint loc_MVP;
    GLuint loc_sampler;

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
        "    outColor = texture(sampler, texCoordFS);\n"
        "}\n";
};

struct ShaderLightDepth
{
    ShaderLightDepth()
    {
        program = createProgram(vertex.c_str(), fragment.c_str());

        loc_u_DepthBiasMVP = glGetUniformLocation(program, "u_DepthBiasMVP");
        loc_viewPos = glGetUniformLocation(program, "viewPos");

        loc_model = glGetUniformLocation(program, "model");
        loc_view = glGetUniformLocation(program, "view");
        loc_projection = glGetUniformLocation(program, "projection");

        loc_material.ambient = glGetUniformLocation(program, "material.ambient");
        loc_material.diffuse = glGetUniformLocation(program, "material.diffuse");
        loc_material.specular = glGetUniformLocation(program, "material.specular");
        loc_material.shininess = glGetUniformLocation(program, "material.shininess");

        loc_light.position = glGetUniformLocation(program, "light.position");
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
        GLuint position;
        GLuint ambient;
        GLuint diffuse;
        GLuint specular;
    } loc_light;

    GLuint program;

    GLint loc_u_DepthBiasMVP;
    GLint loc_viewPos;

    GLint loc_model;
    GLint loc_view;
    GLint loc_projection;

    std::string vertex =
        R"vs(
            #version 300 es
            precision highp float;
            layout(location = )vs" STRV(POS_ATTRIB) R"vs() in vec3 a_pos;
            layout(location = )vs" STRV(NORMAL_ATTRIB) R"vs() in vec3 a_normal;

            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            uniform mat4 u_DepthBiasMVP;

            out vec3 q_pos;
            out vec3 q_normal;
            out vec4 ShadowCoord;

            void main()
            {
                gl_Position = projection * view *  model * vec4(a_pos, 1.0);
                q_pos = vec3(model * vec4(a_pos, 1.0f));
                q_normal = mat3(transpose(inverse(model))) * a_normal;
                ShadowCoord = u_DepthBiasMVP * model * vec4(a_pos, 1.0);
            }
        )vs";

    std::string fragment =
        R"fs(
            #version 300 es
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
                vec3 position;
                vec3 ambient;
                vec3 diffuse;
                vec3 specular;
            };

            uniform Material material;
            uniform Light light;

            uniform sampler2DShadow u_depthTexture;
            uniform vec3 viewPos;

            in vec3 q_pos;
            in vec3 q_normal;
            in vec4 ShadowCoord;

            float GetShadowPCF(in vec4 smcoord)
            {
                float res = 0.0;

                smcoord.z -= 0.00005 * smcoord.w;

                res += textureProjOffset(u_depthTexture, smcoord, ivec2(-1, -1));
                res += textureProjOffset(u_depthTexture, smcoord, ivec2(0, -1));
                res += textureProjOffset(u_depthTexture, smcoord, ivec2(1, -1));
                res += textureProjOffset(u_depthTexture, smcoord, ivec2(-1, 0));
                res += textureProjOffset(u_depthTexture, smcoord, ivec2(0, 0));
                res += textureProjOffset(u_depthTexture, smcoord, ivec2(1, 0));
                res += textureProjOffset(u_depthTexture, smcoord, ivec2(-1, 1));
                res += textureProjOffset(u_depthTexture, smcoord, ivec2(0, 1));
                res += textureProjOffset(u_depthTexture, smcoord, ivec2(1, 1));
                res /= 9.0;

                return res;
            }

            void main()
            {
                // Ambient
                vec3 ambient = light.ambient * material.ambient;

                // Diffuse
                vec3 norm = normalize(q_normal);
                vec3 lightDir = normalize(light.position - q_pos);
                float diff = max(dot(norm, lightDir), 0.0);
                vec3 diffuse = light.diffuse * material.diffuse * diff;

                // Specular
                vec3 viewDir = normalize(viewPos - q_pos);
                vec3 reflectDir = reflect(-lightDir, norm);
                float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
                vec3 specular = light.specular * material.specular * spec;

                float shadow = GetShadowPCF(ShadowCoord);
                out_Color = vec4((ambient + diffuse + specular) * shadow, 1.0);
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
        "    if (color.r + color.g + color.b < 3.0) color = vec4(0.0, 0.0, 0.0, 1.0);\n"
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