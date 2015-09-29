#pragma once

#include <platform.h>
#include <string>
#include <utilities.h>

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


struct ShaderLayered
{
	ShaderLayered()
	{
		program = createProgram(vertex.c_str(), fragment.c_str(), geometry.c_str());
	}

	GLuint program;

	std::string geometry =
		R"gs(
			#version 330
			layout(triangles) in;
			layout(triangle_strip, max_vertices = 96) out;
			out vec3 fColor;

			void main()
			{	
				for (int Layer = 0; Layer < 32; ++Layer)
				{
					gl_Layer = Layer;
 					for (int i = 0; i < 3; ++i)
					{
						gl_Position = gl_in[i].gl_Position;
						fColor = vec3(1.0, Layer / 31.0, 0.0);
						EmitVertex();
					}
					EndPrimitive();
				}
			}
		)gs";
	
	std::string vertex =
		R"vs(
			#version 330
 
			layout (location = 0) in vec2 Position;
 
			void main()
			{
				gl_Position = vec4(Position.xy, 0.0, 1.0);
			}
		)vs";

	std::string fragment =
		R"fs(
			#version 330

			in vec3 fColor;
			out vec4 outColor;

			void main()
			{
				outColor = vec4(fColor, 1.0);
			}
		)fs";
};