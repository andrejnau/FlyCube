#version 300 es
precision highp float;

struct Material
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct Light
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Texture
{
    sampler2D normalMap;
    int has_normalMap;

    sampler2D ambient;
    int has_ambient;

    sampler2D diffuse;
    int has_diffuse;

    sampler2D specular;
    int has_specular;

    sampler2D alpha;
    int has_alpha;
};

uniform sampler2DShadow depthTexture;
uniform int has_depthTexture;
uniform Material material;
uniform Light light;
uniform Texture textures;

in vec3 _FragPos;
in vec3 _Normal;
in vec2 _TexCoords;
in vec3 _Tangent;
in vec3 _Bitangent;
in vec3 _LightPos;
in vec3 _ViewPos;
in vec4 _ShadowCoord;

out vec4 out_Color;

vec3 CalcBumpedNormal()
{
    vec3 normal = texture(textures.normalMap, _TexCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    mat3 tbn = mat3(_Tangent, _Bitangent, _Normal);
    normal = normalize(tbn * normal);
    return normal;
}

float GetShadowPCF(in vec4 smcoord)
{
    float res = 0.0;
    smcoord.z -= 0.0005 * smcoord.w;
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
    vec3 inv_gamma3 = vec3(2.2);

    vec3 normal = normalize(_Normal);
    if (textures.has_normalMap != 0)
        normal = CalcBumpedNormal();

    // Ambient
    vec3 ambient = light.ambient;
    if (textures.has_ambient != 0)
        ambient *= pow(texture(textures.ambient, _TexCoords).rgb, inv_gamma3);
    else
        ambient *= material.ambient;

    // Diffuse
    vec3 lightDir = normalize(_LightPos - _FragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = light.diffuse * diff;
    if (textures.has_diffuse != 0)
        diffuse *= pow(texture(textures.diffuse, _TexCoords).rgb, inv_gamma3);
    else
        diffuse *= material.diffuse;

    // Specular
    vec3 viewDir = normalize(_ViewPos - _FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(normal, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec;
    if (textures.has_specular != 0)
        specular *= texture(textures.specular, _TexCoords).rgb;
    else
        specular *= material.specular;

    float shadow = 1.0;
    if (has_depthTexture != 0)
        shadow = GetShadowPCF(_ShadowCoord);

    vec4 result = vec4(ambient + diffuse * shadow + specular * shadow, 1.0);

    if (textures.has_alpha != 0)
    {
        result.a = texture(textures.alpha, _TexCoords).r;
        if (result.a < 0.5)
            discard;
    }

    out_Color = pow(result, gamma4);
}