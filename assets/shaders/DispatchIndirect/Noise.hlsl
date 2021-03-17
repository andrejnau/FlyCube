cbuffer computeUniformBlock
{
    float time;
}

RWTexture2D<float4> Tex0;

float hash12(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}


[numthreads(8, 8, 1)]
void MainCS(uint3 Gid  : SV_GroupID,          // current group index (dispatched by c++)
            uint3 DTid : SV_DispatchThreadID, // "global" thread id
            uint3 GTid : SV_GroupThreadID,    // current threadId in group / "local" threadId
            uint GI    : SV_GroupIndex)       // "flattened" index of a thread within a group)
{   
    float2 p = float2(DTid.xy + time);
    float output = hash12(p);
    
    Tex0[DTid.xy] = float4(hash12(p), hash12(p + (0.1f).xx), hash12(p + (0.2f).xx), 1.0f);
}