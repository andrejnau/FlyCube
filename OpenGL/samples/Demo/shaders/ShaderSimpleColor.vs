#version 300 es
precision highp float;

#define POS_ATTRIB 0
#define NORMAL_ATTRIB 1
#define TEXTURE_ATTRIB 2
#define TANGENT_ATTRIB 3
#define BITANGENT_ATTRIB 4

layout(location = POS_ATTRIB) in vec3 a_pos;

uniform mat4 u_MVP;

void main()
{
    gl_Position = u_MVP * vec4(a_pos, 1.0);
}