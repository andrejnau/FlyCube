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

uniform Material material;
uniform Light light;
uniform Texture textures;

in vec3 _FragPos;
in vec3 _Normal;
in vec2 _TexCoords;
in vec3 _Tangent;

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gAmbient;
layout (location = 3) out vec3 gDiffuse;
layout (location = 4) out vec4 gSpecular;
layout (location = 5) out vec3 gTangent;

vec3 CalcBumpedNormal()
{
    vec3 T = normalize(_Tangent);
    vec3 N = normalize(_Normal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = normalize(cross(T, N));

    vec3 normal = texture(textures.normalMap, _TexCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    mat3 tbn = mat3(T, B, N);
    normal = normalize(tbn * normal);
    return normal;
}

void main()
{
    if (textures.has_alpha != 0)
    {
        if (texture(textures.alpha, _TexCoords).r < 0.5)
            discard;
    }

    gPosition = _FragPos;

    vec3 normal = normalize(_Normal);
    if (textures.has_normalMap != 0)
        normal = CalcBumpedNormal();

    gNormal = normal;

    gTangent = _Tangent;

    vec3 inv_gamma3 = vec3(2.2);

    // Ambient
    vec3 ambient = light.ambient;
    if (textures.has_ambient != 0)
        ambient *= pow(texture(textures.ambient, _TexCoords).rgb, inv_gamma3);
    else
        ambient *= material.ambient;

    gAmbient = ambient;

    // Diffuse
    vec3 diffuse = light.diffuse;
    if (textures.has_diffuse != 0)
        diffuse *= pow(texture(textures.diffuse, _TexCoords).rgb, inv_gamma3);
    else
        diffuse *= material.diffuse;

    gDiffuse = diffuse;

    // Specular
    vec3 specular = light.specular;
    if (textures.has_specular != 0)
        specular *= texture(textures.specular, _TexCoords).rgb;
    else
        specular *= material.specular;

    gSpecular.rgb = specular;
    gSpecular.a =  material.shininess;
}