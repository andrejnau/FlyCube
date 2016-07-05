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
    input.normal = normalize(input.normal);

    // Diffuse
    float3 lightDir = normalize(input.lightPos - input.fragPos);
    float diff = max(dot(lightDir, input.normal), 0.0);
    float3 kd = float3(0.150257, 0.639282, 0.161245);
    float3 diffuse = kd * diff;

    // Specular
    float3 viewDir = normalize(input.viewPos - input.fragPos);
    float3 reflectDir = reflect(-lightDir, input.normal);
    float spec = pow(max(dot(input.normal, reflectDir), 0.0), 96.0);
    float3 ks = float3(0.5, 0.5, 0.5);
    float3 specular = ks * spec;
    return float4(diffuse + specular, 1.0);
}