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
			layout(location = )vs" STRV(TEXTURE_ATTRIB) R"vs() in vec2 a_texCoord;

			uniform mat4 model;
			uniform mat4 view;
			uniform mat4 projection;

			out vec3 q_pos;
			out vec3 q_normal;
			out vec2 q_texCoord;

			void main()
			{
			    gl_Position = projection * view *  model * vec4(a_pos, 1.0);
				q_pos = vec3(model * vec4(a_pos, 1.0f));
				q_normal = mat3(transpose(inverse(model))) * a_normal;
				q_texCoord = a_texCoord;
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

			uniform vec3 viewPos;

			in vec3 q_pos;
			in vec3 q_normal;
			in vec2 q_texCoord;

			uniform sampler2D texture_diffuse;

			void main()
			{
				// Ambient
				vec3 ambient = light.ambient * material.ambient;

				// Diffuse
				vec3 norm = normalize(q_normal);
				vec3 lightDir = normalize(light.position - q_pos);
				float diff = max(dot(norm, lightDir), 0.0);
				vec3 diffuse = light.diffuse * (diff * material.diffuse);

				// Specular
				vec3 viewDir = normalize(viewPos - q_pos);
				vec3 reflectDir = reflect(-lightDir, norm);
				float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
				vec3 specular = light.specular * (spec * material.specular);

				vec3 result = (ambient + diffuse + specular);
				out_Color = vec4(texture(texture_diffuse, q_texCoord).rgb, 1.0);
			}
		)fs";
};