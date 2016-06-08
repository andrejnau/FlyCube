#pragma once

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
            } vs_out;

            void main()
            {
                gl_Position = projection * view * model * vec4(position, 1.0f);
                vs_out.FragPos = vec3(model * vec4(position, 1.0));
                vs_out.TexCoords = texCoords;
                vs_out.LightPos = lightPos;
                vs_out.ViewPos  = viewPos;

                mat3 normalMatrix = transpose(inverse(mat3(model)));

                vs_out.Tangent = normalize(normalMatrix * tangent);
                vs_out.Bitangent = normalize(normalMatrix * bitangent);
                vs_out.Normal = normalize(normalMatrix * normal);
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

                vec4 result = vec4(ambient + diffuse + specular, 1.0);
                if (textures.has_alpha != 0)
                    result.a = texture(textures.alpha, fs_in.TexCoords).a;

                out_Color = result;
            }
        )fs";
};