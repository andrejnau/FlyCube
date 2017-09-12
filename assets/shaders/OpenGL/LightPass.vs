#version 330

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoords;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

out VertexData
{
   vec2 TexCoords;
};

void main()
{
    gl_Position = vec4(position, 1.0);
    TexCoords = texCoords;
}
