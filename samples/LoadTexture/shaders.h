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