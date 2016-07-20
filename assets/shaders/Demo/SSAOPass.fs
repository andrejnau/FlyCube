#version 300 es
precision highp float;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gTangent;

#define KERNEL_SIZE 64

uniform int num_samples;
uniform mat4 projection;
uniform vec3 samples[KERNEL_SIZE];

float radius = 1.0;

in vec2 TexCoords;

out float out_Color;

void main()
{
    vec3 fragPos = texture(gPosition, TexCoords).rgb;
    vec3 normal = texture(gNormal, TexCoords).rgb;
    vec3 tangent = texture(gTangent, TexCoords).rgb;

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
        float sampleDepth = texture(gPosition, offset.xy).z; // Get depth value of kernel sample

        // range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1.0 - (occlusion / float(KERNEL_SIZE));
    out_Color = occlusion;
}