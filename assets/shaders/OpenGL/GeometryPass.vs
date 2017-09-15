#version 330

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 texCoord;

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
    vec3 tangent;
    vec2 texCoord;
} vs_out;

void main()
{
    vec4 worldPos = model * vec4(position, 1.0);
    vs_out.fragPos = worldPos.xyz;
    vs_out.pos = projection * view * worldPos;
    vs_out.texCoord = texCoord;

    vs_out.normal = mat3(normalMatrix ) * normal;
    vs_out.tangent = mat3(normalMatrix) * tangent;

    gl_Position =  vs_out.pos;
}
