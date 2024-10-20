﻿#include "d3d12.hlsli"
#include "Shader.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"
#include "SSRLib.hlsli"

#define KERNEL_SIZE 16
#define DPETH_MIN_DELTA 0.00001f

cbuffer RootConstants : register(b0, space0)
{
    uint perFrameBufferIndex;
    uint worldNormalIndex;
    uint metallicRoughnessReflectanceAOIndex;
    uint maxDepthBufferIndex;
    uint blueNoiseIndex;
    uint RaycastResultIndex;
    uint MaskResultIndex;
};

SamplerState defaultSampler : register(s10);
SamplerState minDepthSampler : register(s11);

float clampDelta(float2 start, float2 end, float2 delta)
{
    float2 dir = abs(end - start);
    return length(float2(min(dir.x, delta.x), min(dir.y, delta.y)));
}

float4 rayMarch(float3 viewPos, float3 viewDir, float3 screenPos, float2 uv, int numSteps, float thickness, 
    float4x4 projectionMatrix, float4 rayCastSize, float4 zBufferParams, Texture2D<float4> maxDepthTex)
{
    // if(screenPos.z < DPETH_MIN_DELTA)
    // {
    //     return float4(0,0,0,0);
    // }

    float4 rayProj = mul(projectionMatrix, float4(viewPos + viewDir, 1.0f));
    float3 rayDir = normalize(rayProj.xyz / rayProj.w - screenPos);
    rayDir.xy *= 0.5;
    float3 rayStart = float3(uv, screenPos.z);
    float2 screenDelta2 = rayCastSize.zw;
    float d = clampDelta(rayStart.xy, rayStart.xy + rayDir.xy, screenDelta2);
    float3 samplePos = rayStart + rayDir * d;
    int level = 0;
    
    uint mask = 0;
    [loop]
    for (int i = 0; i < numSteps; i++)
    {
        float2 currentDelta = screenDelta2 * exp2(level + 1);
        float distance = clampDelta(samplePos.xy, samplePos.xy + rayDir.xy, currentDelta);
        float3 nextSamplePos = samplePos + rayDir * distance;

        [flatten]
        if(any(nextSamplePos.xy<0) || any(nextSamplePos.xy>=1))
        {
            return float4(samplePos, mask);
        }

        float sampleMaxDepth = maxDepthTex.SampleLevel(minDepthSampler, nextSamplePos.xy, level).r;
        float nextDepth = nextSamplePos.z;
        
        [flatten]
        if (sampleMaxDepth < nextDepth)
        {
            level = min(level + 1, 7);
            samplePos = nextSamplePos;
        }
        else
        {
            level--;
        }
        
        [branch]
        if (level < 0)
        {
            float delta = abs(LinearEyeDepth(samplePos.z, zBufferParams) - LinearEyeDepth(sampleMaxDepth, zBufferParams));
            mask = delta <= thickness && i > 0;
            return float4(samplePos, mask);
        }
    }
    return float4(samplePos, mask);
}

// Brian Karis, Epic Games "Real Shading in Unreal Engine 4"
float4 importanceSampleGGX(float2 Xi, float Roughness)
{
    float m = Roughness * Roughness;
    float m2 = m * m;
		
    float Phi = 2 * F_PI * Xi.x;
				 
    float CosTheta = sqrt((1.0 - Xi.y) / (1.0 + (m2 - 1.0) * Xi.y));
    float SinTheta = sqrt(max(1e-5, 1.0 - CosTheta * CosTheta));
				 
    float3 H;
    H.x = SinTheta * cos(Phi);
    H.y = SinTheta * sin(Phi);
    H.z = CosTheta;
		
    float d = (CosTheta * m2 - CosTheta) * CosTheta + 1;
    float D = m2 / (F_PI * d * d);
    float pdf = D * CosTheta;

    return float4(H, pdf);
}

[numthreads(KERNEL_SIZE, KERNEL_SIZE, 1)]
void CSRaycast(uint3 id : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[perFrameBufferIndex];
    Texture2D<float4> worldNormalMap = ResourceDescriptorHeap[worldNormalIndex];
    Texture2D<float4> mrraMap = ResourceDescriptorHeap[metallicRoughnessReflectanceAOIndex];
    Texture2D<float4> maxDepthMap = ResourceDescriptorHeap[maxDepthBufferIndex];
    Texture2D<float4> blueNoiseMap = ResourceDescriptorHeap[blueNoiseIndex];

    RWTexture2D<float4> RaycastResult = ResourceDescriptorHeap[RaycastResultIndex];
    RWTexture2D<float> MaskResult = ResourceDescriptorHeap[MaskResultIndex];

    float4x4 worldToCameraMatrix = mFrameUniforms.cameraUniform.curFrameUniform.viewFromWorldMatrix;
    float4x4 projectionMatrix = mFrameUniforms.cameraUniform.curFrameUniform.clipFromViewMatrix;
    float4x4 projectionMatrixInv = mFrameUniforms.cameraUniform.curFrameUniform.viewFromClipMatrix;

    float4 zBufferParams = mFrameUniforms.cameraUniform.curFrameUniform.zBufferParams;

    SSRUniform ssrUniform = mFrameUniforms.ssrUniform;

    float4 rayCastSize = ssrUniform.RayCastSize;    
    float2 uv = (id.xy + 0.5) * rayCastSize.zw;

    float3 worldNormal = worldNormalMap.Sample(defaultSampler, uv).rgb * 2.0f - 1.0f;
    float3 viewNormal = normalize(mul((float3x3)worldToCameraMatrix, worldNormal));
    viewNormal.y = -viewNormal.y;

    float roughness = mrraMap.SampleLevel(defaultSampler, uv, 0).y;

    float depth = maxDepthMap.SampleLevel(minDepthSampler, uv, 0).r;
    float3 screenPos = float3(uv * 2 - 1, depth);
    float3 viewPos = getViewPos(screenPos, projectionMatrixInv);

    // float3 norm_o = -normalize(cross(ddy(viewPos), ddx(viewPos)));
    // viewNormal = norm_o;

    // Blue noise generated by https://github.com/bartwronski/BlueNoiseGenerator/
    float2 blueNoiseUV = (uv + ssrUniform.JitterSizeAndOffset.zw) * ssrUniform.ScreenSize.xy * ssrUniform.NoiseSize.zw;
    float2 jitter = blueNoiseMap.SampleLevel(defaultSampler, blueNoiseUV, 0).xy;

    float2 Xi = jitter;
    Xi.y = lerp(Xi.y, 0.0, ssrUniform.BRDFBias);

    float4 H = TangentToWorld(viewNormal, importanceSampleGGX(Xi, roughness));
    float3 dir = reflect(normalize(viewPos), H.xyz);

    int numSteps = ssrUniform.NumSteps;
    float thickness = ssrUniform.Thickness;

    float4 rayTrace = rayMarch(viewPos, dir, screenPos, uv, numSteps, thickness, 
        projectionMatrix, rayCastSize, zBufferParams, maxDepthMap);
    
    float2 rayTraceHit = rayTrace.xy;
    float rayTraceZ = rayTrace.z;
    float rayPDF = H.w;
    float rayMask = rayTrace.w;
    
    RaycastResult[id.xy] = float4(rayTraceHit, rayTraceZ, rayPDF);
    MaskResult[id.xy] = rayMask;
}
