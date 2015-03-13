#pragma once

#include <platform.h>
#include <string>
#include <utils/shaders.h>
#include <utilities.h>

struct ShaderShadow
{
    ShaderShadow()
    {
        program = createProgram(vertex.c_str(), fragment.c_str());
		loc_MVP = glGetUniformLocation(program, "u_MVP");
    }
    
    GLuint program;
    GLint loc_MVP;

    std::string vertex = "#version 300 es\n"
                         "precision mediump float;\n"
                         "layout(location = " STRV(POS_ATTRIB) ") in vec3 a_pos;\n"
                         "uniform mat4 u_MVP;\n"
                         "void main() {\n"
                         "    gl_Position = u_MVP * vec4(a_pos, 1.0);\n"
                         "}\n";

    std::string fragment = "#version 300 es\n"
                            "precision mediump float;\n"
                            "void main()\n" 
                            "{\n"
                            "}\n";
};

struct ShaderSimple
{
	ShaderSimple()
	{
		program = createProgram(vertex.c_str(), fragment.c_str());
		loc_MVP = glGetUniformLocation(program, "u_MVP");
	}

	GLuint program;
	GLint loc_MVP;

	std::string vertex = "#version 300 es\n"
		"precision mediump float;\n"
		"layout(location = " STRV(POS_ATTRIB) ") in vec3 a_pos;\n"
		"uniform mat4 u_MVP;\n"
		"void main() {\n"
		"    gl_Position = u_MVP * vec4(a_pos, 1.0);\n"
		"}\n";

	std::string fragment = "#version 300 es\n"
		"precision mediump float;\n"
		"out vec4 outColor;\n"
		"void main()\n"
		"{\n"
		"    outColor = vec4(1.0, 0.0, 0.0, 1.0);/n"
		"}\n";
};

struct ShaderLight
{
	ShaderLight()
    {
        program = createProgram(vertex.c_str(), fragment.c_str());
        
        loc_MVP = glGetUniformLocation(program, "u_m4MVP");
		loc_DepthBiasMVP = glGetUniformLocation(program, "u_m4DepthBiasMVP");
		loc_lightPosition = glGetUniformLocation(program, "u_lightPosition");
		loc_camera = glGetUniformLocation(program, "u_camera");
		loc_isLight = glGetUniformLocation(program, "u_isLigth");
    }
    
    GLuint program;

    GLint loc_MVP;
    GLint loc_DepthBiasMVP;
    GLint loc_lightPosition;
    GLint loc_camera;  
	GLint loc_isLight;

	std::string vertex =
		"#version 300 es\n"
		"precision mediump float;\n"
		"layout(location = " STRV(POS_ATTRIB) ") in vec3 pos;\n"
		"layout(location = " STRV(NORMAL_ATTRIB) ") in vec3 normal;\n"
		"uniform mat4 u_m4MVP;\n"
		"uniform mat4 u_m4DepthBiasMVP;\n"
		"uniform int u_isLigth;\n"
		"out vec3 modelViewVertex;\n"
		"out vec3 modelViewNormal;\n"
		"out vec4 ShadowCoord;\n"
		"out int isLigth;\n"
		"void main() {\n"
		"    gl_Position = u_m4MVP * vec4(pos, 1.0);\n"
		"    modelViewVertex = vec3(u_m4MVP * vec4(pos, 1.0));\n"
		"    modelViewNormal = normalize(vec3(u_m4MVP * vec4(normal, 0.0)));\n"
		"    ShadowCoord = u_m4DepthBiasMVP * vec4(pos, 1.0);\n"
		"    isLigth = u_isLigth;\n"
        "}\n";

	std::string fragment =
		"#version 300 es\n"
		"precision mediump float;\n"
		"uniform vec3 u_lightPosition;\n"
		"uniform vec3 u_camera;\n"
		"out vec4 outColor;\n"
		"in vec3 modelViewVertex;\n"
		"in vec3 modelViewNormal;\n"
		"in vec4 ShadowCoord;\n"
		"in int isLigth;\n"
		"uniform sampler2D gShadowMap;\n"
		"void main() {\n"
		"    vec3 lightVector = normalize(-u_lightPosition - modelViewVertex);\n"
		"    float diffuse = max(dot(modelViewNormal, lightVector), 0.1);\n"
		"    float distance = length(u_lightPosition - modelViewVertex);\n"
		"    diffuse = diffuse * (1.0 / (1.0 + (0.025 * distance * distance)));\n"
		"    vec3 lookVector = normalize(u_camera - modelViewVertex);\n"
		"    if (isLigth == 0)\n"
		"        diffuse = 1.0;\n"
		"    outColor.rgb = vec3(1.0, 0.0, 0.0) * diffuse;\n"
		"    outColor.a = 1.0;\n"
		"    float Depth = texture(gShadowMap, ShadowCoord.xy).x;\n"
		"	 outColor *= vec4(Depth);\n"
		"}\n";
};