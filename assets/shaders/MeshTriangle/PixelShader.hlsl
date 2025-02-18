struct MeshOutput {
    float4 pos: SV_Position;
    float4 color: COLOR0;
};

float4 main(MeshOutput input) : SV_TARGET
{
   return input.color;
}
