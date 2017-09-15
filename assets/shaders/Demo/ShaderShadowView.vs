#version 300 es
precision highp float;

layout(location = 0) in vec3 pos;
layout(location = 3) in vec2 texCoord;

uniform mat4 u_m4MVP;

out vec2 texCoordFS;

void main()
{
    texCoordFS = texCoord;
    gl_Position = u_m4MVP * vec4(pos, 1.0);
}