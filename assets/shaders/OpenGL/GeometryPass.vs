#version 330

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoords;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

uniform ConstantBuffer
{
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 normalMatrix;
};

out VertexData
{
    vec4 pos;
    vec3 fragPos;
    vec3 normal;
    vec2 texCoord;
    vec3 tangent;
} vs_out;

void main()
{
    vec4 newPosition = vec4(position, 1.0);
    newPosition = model * newPosition;
    vs_out.fragPos = vec3(newPosition);
    newPosition = projection * view * newPosition;

    vs_out.pos = newPosition;
    vs_out.texCoord = texCoords;

    vs_out.normal = normalize(mat3(normalMatrix) * normal);
    vs_out.tangent = normalize(mat3(normalMatrix) * tangent);

    gl_Position =  vs_out.pos;
}
