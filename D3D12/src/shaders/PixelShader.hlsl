struct VS_OUTPUT
{
    float4 pos: SV_POSITION;
    float3 fragPos : POSITION;
    float3 normal : NORMAL;
    float2 texCoord: TEXCOORD;
    float3 tangent: TANGENT;
    float3 bitangent: BITANGENT;
    float3 lightPos : POSITION_LIGHT;
    float3 viewPos : POSITION_VIEW;
};

cbuffer ConstantBuffer : register(b0)
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    float4 lightPos;
    float4 viewPos;
    uint texture_enable;
};

Texture2D diffuseMap : register(t0);
Texture2D normalMap : register(t1);
Texture2D specularMap : register(t2);
SamplerState g_sampler : register(s0);

float4 main(VS_OUTPUT input) : SV_TARGET
{
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent);
    T = normalize(T - dot(T, N) * N);
    float3 B = normalize(cross(T, N));
    float3x3 tbn = float3x3(T, B, N);
    float3 normal = normalMap.Sample(g_sampler, input.texCoord).rgb;
    normal = normalize(2.0 * normal - 1.0);
    input.normal = normalize(mul(normal, tbn));
    // Diffuse
    float3 lightDir = normalize(input.lightPos - input.fragPos);
    float diff = max(dot(lightDir, input.normal), 0.0);
    float3 diffuse_base = float3(1.0, 1.0, 1.0);
    if (texture_enable & (1 << 0))
        diffuse_base = diffuseMap.Sample(g_sampler, input.texCoord).rgb;
    float3 diffuse = diffuse_base * diff;

    // Specular
    float3 viewDir = normalize(input.viewPos - input.fragPos);
    float3 reflectDir = reflect(-lightDir, input.normal);
    float spec = pow(max(dot(input.normal, reflectDir), 0.0), 2048.0);
    float3 specular_base = float3(0.5, 0.5, 0.5);
    if (texture_enable & (1 << 2))
        specular_base = specularMap.Sample(g_sampler, input.texCoord).rgb;
    float3 specular = specular_base * spec;
    return float4(pow(abs(diffuse + specular), 1.0 / 1.33), 1.0);
}