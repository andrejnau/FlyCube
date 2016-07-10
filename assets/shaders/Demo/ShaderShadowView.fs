#version 300 es
precision highp float;

uniform sampler2D sampler;

in vec2 texCoordFS;
out vec4 outColor;

void main()
{
    vec4 color = texture(sampler, texCoordFS);
    outColor = color;
}