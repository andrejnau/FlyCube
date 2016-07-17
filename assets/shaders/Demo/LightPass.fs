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

in vec2 TexCoords;

out vec4 out_Color;

float GetShadowPCF(in vec4 smcoord)
{
    float res = 0.0;
    smcoord.z -= 0.0001 * smcoord.w;
    res += textureProjOffset(depthTexture, smcoord, ivec2(-1, -1));
    res += textureProjOffset(depthTexture, smcoord, ivec2(0, -1));
    res += textureProjOffset(depthTexture, smcoord, ivec2(1, -1));
    res += textureProjOffset(depthTexture, smcoord, ivec2(-1, 0));
    res += textureProjOffset(depthTexture, smcoord, ivec2(0, 0));
    res += textureProjOffset(depthTexture, smcoord, ivec2(1, 0));
    res += textureProjOffset(depthTexture, smcoord, ivec2(-1, 1));
    res += textureProjOffset(depthTexture, smcoord, ivec2(0, 1));
    res += textureProjOffset(depthTexture, smcoord, ivec2(1, 1));
    res /= 9.0;
    return res;
}

vec4 getTextureMultisample(sampler2DMS _texture)
{
    int num_samples = 4;
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
        shadow = GetShadowPCF(shadowCoord);

    float occlusion = 1.0;
    if (has_SSAO != 0)
    {
        occlusion = getTextureMultisample(gSSAO).r;
        occlusion = pow(occlusion, 2.2);
    }

    vec4 result = vec4(ambient * occlusion + diffuse * shadow + specular * shadow, 1.0);
    out_Color = pow(result, gamma4);
}