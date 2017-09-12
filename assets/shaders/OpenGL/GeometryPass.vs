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
};

out VertexData
{
    vec3 _FragPos;
    vec3 _Normal;
    vec3 _Tangent;
    vec2 _TexCoords;
    float _DepthProj;
};

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
