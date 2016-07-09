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
uniform mat4 depthBiasMVP;
uniform vec3 lightPos;
uniform vec3 viewPos;

out vec3 _FragPos;
out vec3 _Normal;
out vec2 _TexCoords;
out vec3 _Tangent;
out vec3 _Bitangent;
out vec3 _LightPos;
out vec3 _ViewPos;
out vec4 _ShadowCoord;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f);
    _FragPos = vec3(model * vec4(position, 1.0f));
    _TexCoords = texCoords;
    _LightPos = lightPos;
    _ViewPos  = viewPos;

    mat3 normalMatrix = mat3(transpose(inverse(model)));
    vec3 T = normalize(normalMatrix * tangent);
    vec3 N = normalize(normalMatrix * normal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(T, N);

    _Tangent = T;
    _Bitangent = B;
    _Normal = N;

    _ShadowCoord = depthBiasMVP * model * vec4(position, 1.0f);
}