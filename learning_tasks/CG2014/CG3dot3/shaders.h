#pragma once

#include <platform.h>
#include <string>
#include <utils/shaders.h>
#include <utilities.h>

struct ShaderLine
{
	ShaderLine()
    {
		program = createProgram(vertex.c_str(), fragment.c_str());
		loc_u_m4MVP = glGetUniformLocation(program, "u_m4MVP");
		uloc_color = glGetUniformLocation(program, "u_color");
    }
    
    GLuint program;

	GLint loc_u_m4MVP;
	GLint uloc_color;

	std::string vertex =
		"#version 300 es\n"
		"precision mediump float;\n"
		"layout(location = " STRV(POS_ATTRIB) ") in vec3 pos;\n"
		"uniform mat4 u_m4MVP;\n"
		"uniform vec4 u_color;\n"
		"out vec4 _color;\n"
		"void main() {\n"
		"    _color = u_color;\n"
		"    gl_Position = u_m4MVP * vec4(pos, 1.0);\n"
		"}\n";

	std::string fragment =
		"#version 300 es\n"
		"precision mediump float;\n"
		"out vec4 outColor;\n"
		"in vec4 _color;\n"
		"void main() {\n"
		"    outColor = _color;\n"
		"}\n";
};