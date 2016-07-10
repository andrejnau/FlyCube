#pragma once

#include <platform.h>
#include <string>
#include <utilities.h>



struct ShaderLight
{
    ShaderLight()
    {
        program = createProgram(vertex.c_str(), fragment.c_str());
        
        loc_model = glGetUniformLocation(program, "model");
        loc_view = glGetUniformLocation(program, "view");
        loc_projection = glGetUniformLocation(program, "projection");
        loc_lightPosition = glGetUniformLocation(program, "u_lightPosition");
        loc_camera = glGetUniformLocation(program, "u_camera");
    }
    
    GLuint program;

    GLint loc_model;
    GLint loc_view;
    GLint loc_projection;
    GLint loc_lightPosition;
    GLint loc_camera;

    std::string vertex = {
        #include "ShaderLight.vs"
    };

    std::string fragment = {
        #include "ShaderLight.fs"
    };
};