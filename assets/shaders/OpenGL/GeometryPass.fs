#version 330

uniform Material
{
    vec3 material_ambient;
    vec3 material_diffuse;
    vec3 material_specular;
    float material_shininess;
};

uniform Light
{
    vec3 light_ambient;
    vec3 light_diffuse;
    vec3 light_specular;
};

uniform sampler2D normalMap;
uniform sampler2D ambientMap;
uniform sampler2D diffuseMap;
uniform sampler2D specularMap;
uniform sampler2D alphaMap;

uniform TexturesEnables
{
    int has_diffuseMap;
    int has_normalMap;
    int has_specularMap;
    int has_glossMap;
    int has_ambientMap;
    int has_alphaMap;
};

in VertexData
{
    vec3 _FragPos;
    vec3 _Normal;
    vec3 _Tangent;
    vec2 _TexCoords;
    float _DepthProj;
};

layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gAmbient;
layout (location = 3) out vec3 gDiffuse;
layout (location = 4) out vec4 gSpecular;

vec3 CalcBumpedNormal()
{
    vec3 T = normalize(_Tangent);
    vec3 N = normalize(_Normal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = normalize(cross(T, N));

    vec3 normal = texture(normalMap, _TexCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    mat3 tbn = mat3(T, B, N);
    normal = normalize(tbn * normal);
    return normal;
}

void main()
{
    if (has_alphaMap != 0)
    {
        if (texture(alphaMap, _TexCoords).r < 0.5)
            discard;
    }

    gPosition = vec4(_FragPos, _DepthProj);

    vec3 normal = normalize(_Normal);
    if (has_normalMap != 0)
        normal = CalcBumpedNormal();

    gNormal = normal;

    vec3 inv_gamma3 = vec3(2.2);

    // Ambient
    vec3 ambient = light_ambient;
    if (has_ambientMap != 0)
        ambient *= pow(texture(ambientMap, _TexCoords).rgb, inv_gamma3);
    else
        ambient *= material_ambient;

    gAmbient = ambient;

    // Diffuse
    vec3 diffuse = light_diffuse;
    if (has_diffuseMap != 0)
        diffuse *= pow(texture(diffuseMap, _TexCoords).rgb, inv_gamma3);
    else
        diffuse *= material_diffuse;

    gDiffuse = diffuse;

    // Specular
    vec3 specular = light_specular;
    if (has_specularMap != 0)
        specular *= texture(specularMap, _TexCoords).rgb;
    else
        specular *= material_specular;

    gSpecular.rgb = specular;
    gSpecular.a =  material_shininess;
}
