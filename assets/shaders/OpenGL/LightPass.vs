#version 330

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 texCoord;

out VertexData
{
   vec2 texCoord;
} vs_out;

void main()
{
    gl_Position = vec4(pos, 1.0);
    vs_out.texCoord = texCoord;
}
