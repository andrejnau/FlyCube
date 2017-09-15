#version 300 es
precision highp float;

layout(location = 0) in vec3 a_pos;

uniform mat4 u_MVP;

out vec3 texCoord;

void main()
{
    gl_Position = u_MVP * vec4(a_pos, 1.0);
    texCoord = normalize(a_pos);
}