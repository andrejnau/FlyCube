#pragma once

#include <platform.h>
#include <string>
#include <utils/shaders.h>
#include <utilities.h>

struct ShaderLight
{
	ShaderLight()
    {
        program = createProgram(vertex.c_str(), fragment.c_str());
        
        loc_MVP = glGetUniformLocation(program, "u_m4MVP");
		loc_lightPosition = glGetUniformLocation(program, "u_lightPosition");
		loc_camera = glGetUniformLocation(program, "u_camera");
    }
    
    GLuint program;

    GLint loc_MVP;
    GLint loc_lightPosition;
    GLint loc_camera;  

	std::string vertex =
		"#version 300 es\n"
		"precision mediump float;\n"
		"layout(location = " STRV(POS_ATTRIB) ") in vec3 pos;\n"
		"layout(location = " STRV(NORMAL_ATTRIB) ") in vec3 normal;\n"
		"uniform mat4 u_m4MVP;\n"
		"uniform mat4 u_m4DepthBiasMVP;\n"
		"out vec3 modelViewVertex;\n"
		"out vec3 modelViewNormal;\n"
		"void main() {\n"
		"    gl_Position = u_m4MVP * vec4(pos, 1.0);\n"
		"    modelViewVertex = vec3(u_m4MVP * vec4(pos, 1.0));\n"
		"    modelViewNormal = normalize(vec3(u_m4MVP * vec4(normal, 0.0)));\n"
        "}\n";

	std::string fragment =
		"#version 300 es\n"
		"precision mediump float;\n"
		"uniform vec3 u_lightPosition;\n"
		"uniform vec3 u_camera;\n"
		"out vec4 outColor;\n"
		"in vec3 modelViewVertex;\n"
		"in vec3 modelViewNormal;\n"
		"void main() {\n"
		"    vec3 lightVector = normalize(-u_lightPosition - modelViewVertex);\n"
		"    float diffuse = max(dot(modelViewNormal, lightVector), 0.1);\n"
		"    float distance = length(u_lightPosition - modelViewVertex);\n"
		"    diffuse = diffuse * (1.0 / (1.0 + (0.00025 * distance * distance)));\n"
		"    vec3 lookVector = normalize(u_camera - modelViewVertex);\n"
		"    outColor.rgb = vec3(1.0, 0.0, 0.0) * diffuse;\n"
		"    outColor.a = 1.0;\n"
		"}\n";
};