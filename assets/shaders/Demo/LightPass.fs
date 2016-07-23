#version 300 es
precision highp float;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAmbient;
uniform sampler2D gDiffuse;
uniform sampler2D gSpecular;
uniform sampler2D gSSAO;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform mat4 depthBiasMVP;

uniform sampler2DShadow depthTexture;
uniform int has_depthTexture;
uniform int has_SSAO;
uniform int num_samples;

in vec2 TexCoords;

out vec4 out_Color;

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
    {
        shadow = 0.0;
        float bias = max(0.001 * (1.0 - dot(normal, lightDir)), 0.0005);
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
        occlusion = pow(texture(gSSAO, TexCoords).r, 2.2);

    vec3 hdrColor  = vec3(ambient * occlusion + diffuse * shadow + specular * shadow);
    /*float exposure = 2.0;
    vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);*/
    out_Color = pow(vec4(hdrColor, 1.0), gamma4);
}