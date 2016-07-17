#version 300 es
precision highp float;

#define POS_ATTRIB 0
#define NORMAL_ATTRIB 1
#define TEXTURE_ATTRIB 2
#define TANGENT_ATTRIB 3
#define BITANGENT_ATTRIB 4

layout(location = POS_ATTRIB) in vec3 position;
layout(location = NORMAL_ATTRIB)in vec3 normal;
layout(location = TEXTURE_ATTRIB) in vec2 texCoords;
layout(location = TANGENT_ATTRIB) in vec3 tangent;
layout(location = BITANGENT_ATTRIB) in vec3 bitangent;

out vec2 TexCoords;

void main()
{
    gl_Position = vec4(position, 1.0);
    TexCoords = texCoords;
}