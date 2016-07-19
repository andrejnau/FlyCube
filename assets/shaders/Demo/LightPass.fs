#version 300 es
precision highp float;
precision highp sampler2DMS;

uniform sampler2DMS gPosition;
uniform sampler2DMS gNormal;
uniform sampler2DMS gAmbient;
uniform sampler2DMS gDiffuse;
uniform sampler2DMS gSpecular;
uniform sampler2DMS gSSAO;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform mat4 depthBiasMVP;

uniform sampler2DShadow depthTexture;
uniform int has_depthTexture;
uniform int has_SSAO;
uniform int num_samples;

in vec2 TexCoords;

out vec4 out_Color;

vec4 getTextureMultisample(sampler2DMS _texture)
{
    ivec2 iTexC = ivec2(TexCoords * vec2(textureSize(_texture)));
    vec4 color = vec4(0.0);
    for (int i = 0; i < num_samples; ++i)
    {
        color += texelFetch(_texture, iTexC, i);
    }
    return color / vec4(num_samples);
}

void main()
{
    vec4 gamma4 = vec4(vec3(1.0/2.2), 1);

    vec3 fragPos = getTextureMultisample(gPosition).rgb;
    vec3 normal = getTextureMultisample(gNormal).rgb;
    vec3 ambient = getTextureMultisample(gAmbient).rgb;
    vec3 diffuse = getTextureMultisample(gDiffuse).rgb;
    vec3 specular = getTextureMultisample(gSpecular).rgb;
    float shininess = getTextureMultisample(gSpecular).a;

    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    diffuse *= diff;

    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(normal, reflectDir), 0.0), shininess);
    specular *= spec;

    vec4 shadowCoord = depthBiasMVP * vec4(fragPos, 1.0);

    float shadow = 1.0;
    if (has_depthTexture != 0)
    {
        shadow = 0.0;
        float bias = max(0.001 * (1.0 - dot(normal, lightDir)), 0.0001);
        shadowCoord.xyz /= shadowCoord.w;
        shadowCoord.z -= bias * shadowCoord.w;
        shadow += textureProjOffset(depthTexture, shadowCoord, ivec2(-1, -1));
        shadow += textureProjOffset(depthTexture, shadowCoord, ivec2(0, -1));
        shadow += textureProjOffset(depthTexture, shadowCoord, ivec2(1, -1));
        shadow += textureProjOffset(depthTexture, shadowCoord, ivec2(-1, 0));
        shadow += textureProjOffset(depthTexture, shadowCoord, ivec2(0, 0));
        shadow += textureProjOffset(depthTexture, shadowCoord, ivec2(1, 0));
        shadow += textureProjOffset(depthTexture, shadowCoord, ivec2(-1, 1));
        shadow += textureProjOffset(depthTexture, shadowCoord, ivec2(0, 1));
        shadow += textureProjOffset(depthTexture, shadowCoord, ivec2(1, 1));
        shadow /= 9.0;
    }

    float occlusion = 1.0;
    if (has_SSAO != 0)
    {
        occlusion = getTextureMultisample(gSSAO).r;
        occlusion = pow(occlusion, 2.2);
    }

    vec3 hdrColor  = vec3(ambient * occlusion + diffuse * shadow + specular * shadow);
    /*float exposure = 2.0;
    vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);*/
    out_Color = pow(vec4(hdrColor, 1.0), gamma4);
}