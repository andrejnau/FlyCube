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

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 _FragPos;
out vec3 _Normal;
out vec2 _TexCoords;
out vec3 _Tangent;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);
    _FragPos = vec3(model * vec4(position, 1.0));
    _TexCoords = texCoords;

    mat3 normalMatrix = mat3(transpose(inverse(model)));
    _Normal = normalize(normalMatrix * normal);
    _Tangent = normalize(normalMatrix * tangent);
}