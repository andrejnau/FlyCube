#version 300 es
precision highp float;
precision highp sampler2DMS;

uniform sampler2DMS gPosition;
uniform sampler2DMS gNormal;
uniform sampler2DMS gTangent;

#define KERNEL_SIZE 64

uniform mat4 projection;
uniform vec3 samples[KERNEL_SIZE];

float radius = 1.0;

in vec2 TexCoords;

out float out_Color;

vec4 getTextureMultisample(sampler2DMS _texture, vec2 texCoords)
{
    int num_samples = 4;
    ivec2 iTexC = ivec2(texCoords * vec2(textureSize(_texture)));
    vec4 color = vec4(0.0);
    for (int i = 0; i < num_samples; ++i)
    {
        color += texelFetch(_texture, iTexC, i);
    }
    return color / vec4(num_samples);
}

void main()
{
    vec3 fragPos = getTextureMultisample(gPosition, TexCoords).rgb;
    vec3 normal = getTextureMultisample(gNormal, TexCoords).rgb;
    vec3 tangent = getTextureMultisample(gTangent, TexCoords).rgb;

    tangent = normalize(tangent - normal * dot(tangent, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for(int i = 0; i < KERNEL_SIZE; ++i)
    {
        // get sample position
        vec3 samplePos = fragPos + TBN * (samples[i] * radius);

        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

        // get sample depth
        float sampleDepth = getTextureMultisample(gPosition, offset.xy).z; // Get depth value of kernel sample

        // range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1.0 - (occlusion / float(KERNEL_SIZE));
    out_Color = occlusion;
}