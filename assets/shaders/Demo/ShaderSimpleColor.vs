#version 300 es
precision highp float;

layout(location = 0) in vec3 a_pos;

uniform mat4 u_MVP;

void main()
{
    gl_Position = u_MVP * vec4(a_pos, 1.0);
}