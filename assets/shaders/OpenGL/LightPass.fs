#version 330

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAmbient;
uniform sampler2D gDiffuse;
uniform sampler2D gSpecular;

uniform ConstantBuffer
{
    vec3 lightPos;
    vec3 viewPos;
};

in VertexData
{
   vec2 TexCoords;
};

layout (location = 0) out vec4 out_Color;

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

    vec3 hdrColor  = vec3(ambient + diffuse  + specular);
    out_Color = pow(vec4(hdrColor, 1.0), gamma4);
}
