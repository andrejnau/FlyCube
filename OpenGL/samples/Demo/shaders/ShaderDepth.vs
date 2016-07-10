#version 300 es
precision highp float;

#define POS_ATTRIB 0
#define NORMAL_ATTRIB 1
#define TEXTURE_ATTRIB 2
#define TANGENT_ATTRIB 3
#define BITANGENT_ATTRIB 4

layout(location =  POS_ATTRIB) in vec3 pos;

uniform mat4 u_m4MVP;

void main()
{
    gl_Position = u_m4MVP * vec4(pos, 1.0);
}