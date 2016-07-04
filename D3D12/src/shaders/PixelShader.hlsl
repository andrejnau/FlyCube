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

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

float4 main(VS_OUTPUT input) : SV_TARGET
{
    // Diffuse
    float3 lightDir = normalize(input.lightPos - input.fragPos);
    float diff = max(dot(lightDir, input.normal), 0.0);
    float3 diffuse = g_texture.Sample(g_sampler, input.texCoord) * diff;

    // Specular
    float3 viewDir = normalize(input.viewPos - input.fragPos);
    float3 reflectDir = reflect(-lightDir, input.normal);
    float spec = pow(max(dot(input.normal, reflectDir), 0.0), 32.0);
    float3 specular = float3(1.0, 1.0, 1.0) * spec;
    return float4(diffuse + specular, 1.0);
}