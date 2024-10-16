#include "ShaderUtility.hlsli"

struct VertexAttributes
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

cbuffer RootConstants : register(b0, space0) { uint texIndex; };

SamplerState defaultSampler : register(s10);

float4 PSMain(VertexAttributes input) : SV_Target0
{
    Texture2D<float4> baseColorTex = ResourceDescriptorHeap[texIndex];

    //float4 linearRGB = baseColorTex.Sample(defaultSampler, float2(input.texcoord.x, 1 - input.texcoord.y));
    float4 linearRGB = baseColorTex.Sample(defaultSampler, input.texcoord.xy);

    return float4(ApplyDisplayProfile(linearRGB.rgb, DISPLAY_PLANE_FORMAT), 1.0f);
}
