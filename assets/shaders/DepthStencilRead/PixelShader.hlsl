struct VsOutput
{
    float4 pos      : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

struct ConstantLayout
{
    uint2 screen_size;
};

ConstantBuffer<ConstantLayout> cbv : register(b1, space0);
Texture2D<float> depth_buffer : register(t2, space0);
Texture2D<uint2> stencil_buffer : register(t3, space0);

float GetDepth(float2 texcoord) {
    float depth = depth_buffer.Load(int3(texcoord * cbv.screen_size, 0));
    return pow(depth, 8.0);
}

float GetStencil(float2 texcoord) {
    const uint kMaxStencilValue = 15;
    uint2 stencil = stencil_buffer.Load(int3(texcoord * cbv.screen_size, 0));
    return stencil.r / float(kMaxStencilValue);
}

float4 main(VsOutput input) : SV_TARGET
{
    float depth = GetDepth(input.texcoord);
    float stencil = GetStencil(input.texcoord - float2(0.5, 0.0));
    float4 left = float4(depth, depth, depth, 1.0);
    float4 right = float4(stencil, stencil, stencil, 1.0);
    return lerp(left, right, step(0.5, input.texcoord.x));
}
