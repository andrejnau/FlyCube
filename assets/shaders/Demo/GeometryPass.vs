#version 300 es
precision highp float;

layout(location = 0) in vec3 position;
layout(location = 1)in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 texCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 _FragPos;
out vec3 _Normal;
out vec3 _Tangent;
out vec2 _TexCoords;
out float _DepthProj;

void main()
{
    vec4 viewPos = view * model * vec4(position, 1.0);
    _FragPos = viewPos.xyz;
    gl_Position = projection * viewPos;
    _DepthProj = gl_Position.z;
    _TexCoords = texCoords;

    mat3 normalMatrix = transpose(inverse(mat3(view * model)));
    _Normal = normalize(normalMatrix * normal);
    _Tangent = normalize(normalMatrix * tangent);
}