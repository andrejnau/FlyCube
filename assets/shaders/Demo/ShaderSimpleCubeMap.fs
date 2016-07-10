#version 300 es
precision highp float;

uniform samplerCube cubemap;

in vec3 texCoord;
out vec4 outColor;

void main()
{
    outColor = texture(cubemap, texCoord);
}