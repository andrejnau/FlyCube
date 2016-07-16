#version 300 es
precision highp float;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAmbient;
uniform sampler2D gDiffuse;
uniform sampler2D gSpecular;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform mat4 depthBiasMVP;

uniform sampler2DShadow depthTexture;
uniform int has_depthTexture;

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
    return max(0.25, res);
}

void main()
{
    vec4 gamma4 = vec4(vec3(1.0/2.2), 1);

    vec3 fragPos = texture(gPosition, TexCoords).rgb;
    vec3 normal = texture(gNormal, TexCoords).rgb;
    vec3 ambient = texture(gAmbient, TexCoords).rgb;
    vec3 diffuse = texture(gDiffuse, TexCoords).rgb;
    vec3 specular = texture(gSpecular, TexCoords).rgb;
    float shininess = texture(gSpecular, TexCoords).a;

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

    vec4 result = vec4(ambient + diffuse * shadow + specular * shadow, 1.0);

    out_Color = pow(result, gamma4);
}