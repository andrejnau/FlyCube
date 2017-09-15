#version 300 es
precision highp float;

layout(location =  0) in vec3 pos;

uniform mat4 u_m4MVP;

void main()
{
    gl_Position = u_m4MVP * vec4(pos, 1.0);
}