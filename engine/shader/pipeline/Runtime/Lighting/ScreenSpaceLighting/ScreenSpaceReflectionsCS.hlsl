
// Tweak parameters.
// #define DEBUG
#define SSR_TRACE_BEHIND_OBJECTS
#define SSR_TRACE_TOWARDS_EYE
#ifndef SSR_APPROX
    #define SAMPLES_VNDF
#endif
#define SSR_TRACE_EPS               0.000488281f // 2^-11, should be good up to 4K
#define MIN_GGX_ROUGHNESS           0.00001f
#define MAX_GGX_ROUGHNESS           0.99999f

//--------------------------------------------------------------------------------------------------
// Included headers
//--------------------------------------------------------------------------------------------------

#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/Packing.hlsl"
#include "../../ShaderLibrary/Color.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"
#include "../../Lighting//ScreenSpaceLighting/ScreenSpaceLighting.hlsl"

#include "../../Material/NormalBuffer.hlsl"

#include "../../ShaderLibrary/ImageBasedLighting.hlsl"
#include "../../RenderPipeline/Raytracing/Shaders/RaytracingSampling.hlsl"
#include "../../RenderPipeline/Raytracing/Shaders/RayTracingCommon.hlsl"
#include "../../Material/PreIntegratedFGD/PreIntegratedFGD.hlsl"
#include "../../Material/Builtin/BuiltinData.hlsl"

#ifndef FGDTEXTURE_RESOLUTION
#define FGDTEXTURE_RESOLUTION (64)
#endif

//--------------------------------------------------------------------------------------------------
// Inputs & outputs
//--------------------------------------------------------------------------------------------------

// For opaque we do the following operation:
// - Render opaque object in depth buffer
// - Generate depth pyramid from opaque depth buffer
// - Trigger ray from position recover from depth pyramid and raymarch with depth pyramid
// For transparent reflection we chose to not regenerate a depth pyramid to save performance. So we have
// - Generate depth pyramid from opaque depth buffer
// - Trigger ray from position recover from depth buffer (use depth pyramid) and raymarch with depth pyramid
// - Render transparent object with reflection in depth buffer in transparent prepass
// - Trigger ray from position recover from new depth buffer and raymarch with opaque depth pyramid
// So we need a seperate texture for the mip chain and for the depth source when doing the transprent reflection

struct SSRStruct
{
    float _SsrThicknessScale;
    float _SsrThicknessBias;
    int _SsrStencilBit;
    int _SsrIterLimit;
    
    float _SsrRoughnessFadeEnd;
    float _SsrRoughnessFadeRcpLength;
    float _SsrRoughnessFadeEndTimesRcpLength;
    float _SsrEdgeFadeRcpLength;

    int _SsrDepthPyramidMaxMip;
    int _SsrColorPyramidMaxMip;
    int _SsrReflectsSky;
    float _SsrAccumulationAmount;
    
    float _SsrPBRSpeedRejection;
    float _SsrPBRBias;
    float _SsrPRBSpeedRejectionScalerFactor;
    float _SsrPBRPad0;
};

SamplerState s_point_clamp_sampler      : register(s10);
SamplerState s_linear_clamp_sampler     : register(s11);
SamplerState s_linear_repeat_sampler    : register(s12);
SamplerState s_trilinear_clamp_sampler  : register(s13);
SamplerState s_trilinear_repeat_sampler : register(s14);

//--------------------------------------------------------------------------------------------------
// Helpers
//--------------------------------------------------------------------------------------------------

// Weight for SSR where Fresnel == 1 (returns value/pdf)
float GetSSRSampleWeight(float3 V, float3 L, float roughness)
{
    // Simplification:
    // value = D_GGX / (lambdaVPlusOne + lambdaL);
    // pdf = D_GGX / lambdaVPlusOne;

    const float lambdaVPlusOne = Lambda_GGX(roughness, V) + 1.0;
    const float lambdaL = Lambda_GGX(roughness, L);

    return lambdaVPlusOne / (lambdaVPlusOne + lambdaL);
}

float Normalize01(float value, float minValue, float maxValue)
{
    return (value - minValue) / (maxValue - minValue);
}

// Specialization without Fresnel (see PathTracingBSDF.hlsl for the reference implementation)
bool SampleGGX_VNDF(float roughness_,
                    float3x3 localToWorld,
                    float3 V,
                    float2 inputSample,
                out float3 outgoingDir,
                out float weight)
{
    weight = 0.0f;

    float roughness = clamp(roughness_, MIN_GGX_ROUGHNESS, MAX_GGX_ROUGHNESS);

    float VdotH;
    float3 localV, localH;
    SampleGGXVisibleNormal(inputSample.xy, V, localToWorld, roughness, localV, localH, VdotH);

    // Compute the reflection direction
    float3 localL = 2.0 * VdotH * localH - localV;
    outgoingDir = mul(localL, localToWorld);

    if (localL.z < 0.001)
    {
        return false;
    }

    weight = GetSSRSampleWeight(localV, localL, roughness);

    if (weight < 0.001)
        return false;

    return true;
}

float PerceptualRoughnessFade(float perceptualRoughness, float fadeRcpLength, float fadeEndTimesRcpLength)
{
    float t = Remap10(perceptualRoughness, fadeRcpLength, fadeEndTimesRcpLength);
    return Smoothstep01(t);
}

#ifdef SSR_TRACE
void GetHitInfos(
    FrameUniforms frameUniform, float4 _RTHandleScale, int _FrameCount, float _SsrPBRBias, float4 _ScreenSize,
    Texture2D<float4> normalBufferTexture, Texture2D<float> _CameraDepthTexture, Texture2D<float> _OwenScrambledTexture, Texture2D<float> _ScramblingTileXSPP, Texture2D<float> _RankingTileXSPP,
    uint2 positionSS, out float srcPerceptualRoughness, out float3 positionWS, out float weight, out float3 N, out float3 L, out float3 V, out float NdotL, out float NdotH, out float VdotH, out float NdotV)
{
    float2 uv = float2(positionSS) * _RTHandleScale.xy;

    float2 Xi;
    Xi.x = GetBNDSequenceSample(_OwenScrambledTexture, _ScramblingTileXSPP, _RankingTileXSPP, positionSS, _FrameCount, 0);
    Xi.y = GetBNDSequenceSample(_OwenScrambledTexture, _ScramblingTileXSPP, _RankingTileXSPP, positionSS, _FrameCount, 1);

    NormalData normalData;
    DecodeFromNormalBuffer(normalBufferTexture, positionSS, normalData);

    srcPerceptualRoughness = normalData.perceptualRoughness;

    float roughness = PerceptualRoughnessToRoughness(normalData.perceptualRoughness);
    float3x3 localToWorld = GetLocalFrame(normalData.normalWS);

    float coefBias = _SsrPBRBias / roughness;
    Xi.x = lerp(Xi.x, 0.0f, roughness * coefBias);

#ifdef DEPTH_SOURCE_NOT_FROM_MIP_CHAIN
    float  deviceDepth = LOAD_TEXTURE2D_X(_DepthTexture, positionSS).r;
#else
    float  deviceDepth = LOAD_TEXTURE2D(_CameraDepthTexture, positionSS).r;
#endif

    float2 positionNDC = positionSS * _ScreenSize.zw + (0.5 * _ScreenSize.zw);
    positionWS = ComputeWorldSpacePosition(positionNDC, deviceDepth, UNITY_MATRIX_I_VP(frameUniform.cameraUniform));
    V = GetWorldSpaceNormalizeViewDir(frameUniform, positionWS);

    N = normalData.normalWS;

#ifdef SAMPLES_VNDF
    float value;

    SampleGGX_VNDF(roughness,
        localToWorld,
        V,
        Xi,
        L,
        weight);

    NdotV = dot(normalData.normalWS, V);
    NdotL = dot(normalData.normalWS, L);
    float3 H = normalize(V + L);
    NdotH = dot(normalData.normalWS, H);
    VdotH = dot(V, H);
#else
    SampleGGXDir(Xi, V, localToWorld, roughness, L, NdotL, NdotH, VdotH);

    NdotV = dot(normalData.normalWS, V);
    float Vg = V_SmithJointGGX(NdotL, NdotV, roughness);

    weight = 4.0f * NdotL * VdotH * Vg / NdotH;
#endif
}
#endif

float2 GetHitNDC(Texture2D<float4> _CameraMotionVectorsTexture, float4 _ScreenSize, float4 _RTHandleScale, float2 positionNDC)
{
    // TODO: it's important to account for occlusion/disocclusion to avoid artifacts in motion.
    // This would require keeping the depth buffer from the previous frame.
    float2 motionVectorNDC;
    DecodeMotionVector(SAMPLE_TEXTURE2D_LOD(_CameraMotionVectorsTexture, s_linear_clamp_sampler, min(positionNDC, 1.0f - 0.5f * _ScreenSize.zw) * _RTHandleScale.xy, 0), motionVectorNDC);
    float2 prevFrameNDC = positionNDC - motionVectorNDC;
    return prevFrameNDC;
}

float3 GetWorldSpacePosition(FrameUniforms frameUniform, Texture2D<float> _CameraDepthTexture, float4 _RTHandleScale, uint2 positionSS)
{
    float4 _ScreenSize = frameUniform.baseUniform._ScreenSize;
    
    float2 uv = float2(positionSS) * _RTHandleScale.xy;

#ifdef DEPTH_SOURCE_NOT_FROM_MIP_CHAIN
    float  deviceDepth = LOAD_TEXTURE2D_X(_DepthTexture, positionSS).r;
#else
    float  deviceDepth = LOAD_TEXTURE2D(_CameraDepthTexture, positionSS).r;
#endif

    float2 positionNDC = positionSS *_ScreenSize.zw + (0.5 * _ScreenSize.zw);

    return ComputeWorldSpacePosition(positionNDC, deviceDepth, UNITY_MATRIX_I_VP(frameUniform.cameraUniform));
}

float2 GetWorldSpacePoint(
    FrameUniforms frameUniform, Texture2D<float> _CameraDepthTexture, Texture2D<float4> _SsrHitPointTexture,
    float4 _RTHandleScale, uint2 positionSS, out float3 positionSrcWS, out float3 positionDstWS)
{
    float4 _ScreenSize = frameUniform.baseUniform._ScreenSize;
    
    positionSrcWS = GetWorldSpacePosition(frameUniform, _CameraDepthTexture, _RTHandleScale, positionSS);

    float2 hitData = _SsrHitPointTexture[positionSS].xy;
    uint2 positionDstSS = (hitData.xy - (0.5 * _ScreenSize.zw)) / _ScreenSize.zw;

    positionDstWS = GetWorldSpacePosition(frameUniform, _CameraDepthTexture, _RTHandleScale, positionDstSS);

    return hitData.xy;
}

float3 GetHitColor(
    Texture2D<float4> _CameraMotionVectorsTexture, Texture2D<float4> _ColorPyramidTexture, 
    float4 _ScreenSize, float4 _RTHandleScale, float4 _ColorPyramidUvScaleAndLimitPrevFrame,
    float _SsrRoughnessFadeRcpLength, float _SsrRoughnessFadeEndTimesRcpLength, float _SsrEdgeFadeRcpLength,
    float2 hitPositionNDC, float perceptualRoughness, out float opacity, int mipLevel = 0)
{
    float2 prevFrameNDC = GetHitNDC(_CameraMotionVectorsTexture, _ScreenSize, _RTHandleScale, hitPositionNDC);
    float2 prevFrameUV = prevFrameNDC * _ColorPyramidUvScaleAndLimitPrevFrame.xy;

    float tmpCoef = PerceptualRoughnessFade(perceptualRoughness, _SsrRoughnessFadeRcpLength, _SsrRoughnessFadeEndTimesRcpLength);
    opacity = EdgeOfScreenFade(prevFrameNDC, _SsrEdgeFadeRcpLength) * tmpCoef;
    return SAMPLE_TEXTURE2D_LOD(_ColorPyramidTexture, s_trilinear_clamp_sampler, prevFrameUV, mipLevel).rgb;
}

float2 GetSampleInfo(
    FrameUniforms frameUniform, Texture2D<float4> _SsrHitPointTexture, Texture2D<float> _CameraDepthTexture,
    Texture2D<float4> _CameraMotionVectorsTexture, Texture2D<float4> _ColorPyramidTexture, Texture2D<float4> _NormalTexture,
    float4 _ScreenSize, float4 _ColorPyramidUvScaleAndLimitPrevFrame, float _SsrRoughnessFadeRcpLength,
    float _SsrRoughnessFadeEndTimesRcpLength, float _SsrEdgeFadeRcpLength,
    float4 _RTHandleScale, uint2 positionSS, out float3 color, out float weight, out float opacity)
{
    float3 positionSrcWS;
    float3 positionDstWS;
    float2 hitData = GetWorldSpacePoint(frameUniform, _CameraDepthTexture,
        _SsrHitPointTexture, _RTHandleScale, positionSS, positionSrcWS, positionDstWS);

    float3 V = GetWorldSpaceNormalizeViewDir(frameUniform, positionSrcWS);
    float3 L = normalize(positionDstWS - positionSrcWS);
    float3 H = normalize(V + L);

    NormalData normalData;
    DecodeFromNormalBuffer(_NormalTexture, positionSS, normalData);

    float roughness = PerceptualRoughnessToRoughness(normalData.perceptualRoughness);

    roughness = clamp(roughness, MIN_GGX_ROUGHNESS, MAX_GGX_ROUGHNESS);

    weight = GetSSRSampleWeight(V, L, roughness);

    color = GetHitColor(_CameraMotionVectorsTexture, _ColorPyramidTexture, _ScreenSize, _RTHandleScale,
        _ColorPyramidUvScaleAndLimitPrevFrame, _SsrRoughnessFadeRcpLength, _SsrRoughnessFadeEndTimesRcpLength,
        _SsrEdgeFadeRcpLength, hitData.xy, normalData.perceptualRoughness, opacity, 0);

    return hitData;
}

void GetNormalAndPerceptualRoughness(Texture2D<float4> _NormalTexture, uint2 positionSS, out float3 normalWS, out float perceptualRoughness)
{
    // Load normal and perceptualRoughness.
    NormalData normalData;
    DecodeFromNormalBuffer(_NormalTexture, positionSS, normalData);
    normalWS = normalData.normalWS;
    // float4 packedCoatMask = _SsrClearCoatMaskTexture[positionSS];
    // perceptualRoughness = HasClearCoatMask(packedCoatMask) ? CLEAR_COAT_SSR_PERCEPTUAL_ROUGHNESS : normalData.perceptualRoughness;
    perceptualRoughness = normalData.perceptualRoughness;
}

void WriteDebugInfo(uint2 positionSS, float4 value)
{
#ifdef DEBUG
    _SsrDebugTexture[positionSS] = value;
#endif
}

//--------------------------------------------------------------------------------------------------
// Implementation
//--------------------------------------------------------------------------------------------------

#define USE_COARSE_STENCIL 0
#ifdef SSR_TRACE


cbuffer RootConstants : register(b0, space0)
{
    uint frameUniformIndex;
    uint ssrStructBufferIndex;
    uint normalBufferIndex;
    uint depthTextureIndex;
    uint cameraMotionVectorsTextureIndex;
    uint SsrHitPointTextureIndex;
    
    uint OwenScrambledTextureIndex;
    uint ScramblingTileXSPPIndex;
    uint RankingTileXSPPIndex;
};

[numthreads(8, 8, 1)]
void ScreenSpaceReflectionsTracing(uint3 groupId          : SV_GroupID,
                                   uint3 dispatchThreadId : SV_DispatchThreadID)
{
    ConstantBuffer<FrameUniforms> frameUniform = ResourceDescriptorHeap[frameUniformIndex];
    ConstantBuffer<SSRStruct> ssrStruct = ResourceDescriptorHeap[ssrStructBufferIndex];
    Texture2D<float4> normalTexture = ResourceDescriptorHeap[normalBufferIndex];
    Texture2D<float> depthTexture = ResourceDescriptorHeap[depthTextureIndex];
    Texture2D<float4> cameraMotionVectorsTexture = ResourceDescriptorHeap[cameraMotionVectorsTextureIndex];
    
    RWTexture2D<float2> _SsrHitPointTexture = ResourceDescriptorHeap[SsrHitPointTextureIndex];
    
    Texture2D<float> _OwenScrambledTexture = ResourceDescriptorHeap[OwenScrambledTextureIndex];
    Texture2D<float> _ScramblingTileXSPP = ResourceDescriptorHeap[ScramblingTileXSPPIndex];
    Texture2D<float> _RankingTileXSPP = ResourceDescriptorHeap[RankingTileXSPPIndex];
    
    CameraUniform cameraUniform = frameUniform.cameraUniform;

    float4 _ScreenSize = frameUniform.baseUniform._ScreenSize;
    int _FrameCount = frameUniform.baseUniform._FrameCount;


    float _SsrThicknessScale = ssrStruct._SsrThicknessScale;
    float _SsrThicknessBias = ssrStruct._SsrThicknessBias;
    int _SsrStencilBit = ssrStruct._SsrStencilBit;
    int _SsrIterLimit = ssrStruct._SsrIterLimit;

    float _SsrRoughnessFadeEnd = ssrStruct._SsrRoughnessFadeEnd;
    float _SsrRoughnessFadeRcpLength = ssrStruct._SsrRoughnessFadeRcpLength;
    float _SsrRoughnessFadeEndTimesRcpLength = ssrStruct._SsrRoughnessFadeEndTimesRcpLength;
    float _SsrEdgeFadeRcpLength = ssrStruct._SsrEdgeFadeRcpLength;

    int _SsrDepthPyramidMaxMip = ssrStruct._SsrDepthPyramidMaxMip;
    int _SsrColorPyramidMaxMip = ssrStruct._SsrColorPyramidMaxMip;
    int _SsrReflectsSky = ssrStruct._SsrReflectsSky;
    float _SsrAccumulationAmount = ssrStruct._SsrAccumulationAmount;

    float _SsrPBRSpeedRejection = ssrStruct._SsrPBRSpeedRejection;
    float _SsrPBRBias = ssrStruct._SsrPBRBias;
    float _SsrPRBSpeedRejectionScalerFactor = ssrStruct._SsrPRBSpeedRejectionScalerFactor;
    float _SsrPBRPad0 = ssrStruct._SsrPBRPad0;

    float4 _RTHandleScale = float4(1,1,1,1);
    
    uint2 positionSS = dispatchThreadId.xy;

    bool doesntReceiveSSR = false;

    // NOTE: Currently we profiled that generating the HTile for SSR and using it is not worth it the optimization.
    // However if the generated HTile will be used for something else but SSR, this should be re-enabled and in C#.
#if USE_COARSE_STENCIL
    // Check HTile first. Note htileAddress should be already in scalar before WaveReadLaneFirst, but forcing it to be sure.
    // TODO: Verify the need of WaveReadLaneFirst
    uint htileAddress = WaveReadLaneFirst(Get1DAddressFromPixelCoord(groupId.xy, _CoarseStencilBufferSize.xy, groupId.z));
    uint htileValue   = _CoarseStencilBuffer[htileAddress];

    doesntReceiveSSR = (htileValue & _SsrStencilBit) == 0;
    if (doesntReceiveSSR)
    {
        WriteDebugInfo(positionSS, -1);
        return;
    }
#endif

    // uint stencilValue = GetStencilValue(LOAD_TEXTURE2D(_StencilTexture, dispatchThreadId.xy));
    // doesntReceiveSSR = (stencilValue & _SsrStencilBit) == 0;
    // if (doesntReceiveSSR)
    // {
    //     WriteDebugInfo(positionSS, -1);
    //     return;
    // }

    NormalData normalData;
    DecodeFromNormalBuffer(normalTexture, positionSS, normalData);

    float deviceDepth = LOAD_TEXTURE2D(depthTexture, positionSS).r;

#ifdef SSR_APPROX
    float2 positionNDC = positionSS * _ScreenSize.zw + (0.5 * _ScreenSize.zw); // Should we precompute the half-texel bias? We seem to use it a lot.
    float3 positionWS = ComputeWorldSpacePosition(positionNDC, deviceDepth, UNITY_MATRIX_I_VP); // Jittered
    float3 V = GetWorldSpaceNormalizeViewDir(positionWS);

    float3 N;
    float perceptualRoughness;
    GetNormalAndPerceptualRoughness(positionSS, N, perceptualRoughness);

    float3 R = reflect(-V, N);
#else
    float weight;
    float NdotL, NdotH, VdotH, NdotV;
    float3 R, V, N;
    float3 positionWS;
    float perceptualRoughness;
    GetHitInfos(
        frameUniform, _RTHandleScale, _FrameCount, _SsrPBRBias, _ScreenSize,
        normalTexture, depthTexture, _OwenScrambledTexture, _ScramblingTileXSPP, _RankingTileXSPP, 
        positionSS, perceptualRoughness, positionWS, weight, N, R, V, NdotL, NdotH, VdotH, NdotV);

    if (NdotL < 0.001f || weight < 0.001f)
    {
        WriteDebugInfo(positionSS, -1);
        return;
    }
#endif

    float3 camPosWS = GetCurrentViewPosition(frameUniform);

    // Apply normal bias with the magnitude dependent on the distance from the camera.
    // Unfortunately, we only have access to the shading normal, which is less than ideal...
    positionWS = camPosWS + (positionWS - camPosWS) * (1 - 0.001 * rcp(max(dot(N, V), FLT_EPS)));
    deviceDepth = ComputeNormalizedDeviceCoordinatesWithZ(positionWS, UNITY_MATRIX_VP(cameraUniform)).z;
    bool killRay = deviceDepth == UNITY_RAW_FAR_CLIP_VALUE;

    // Ref. #1: Michal Drobot - Quadtree Displacement Mapping with Height Blending.
    // Ref. #2: Yasin Uludag  - Hi-Z Screen-Space Cone-Traced Reflections.
    // Ref. #3: Jean-Philippe Grenier - Notes On Screen Space HIZ Tracing.
    // Warning: virtually all of the code below assumes reverse Z.

    // We start tracing from the center of the current pixel, and do so up to the far plane.
    float3 rayOrigin = float3(positionSS + 0.5, deviceDepth);

    float3 reflPosWS  = positionWS + R;
    float3 reflPosNDC = ComputeNormalizedDeviceCoordinatesWithZ(reflPosWS, UNITY_MATRIX_VP(cameraUniform)); // Jittered
    float3 reflPosSS  = float3(reflPosNDC.xy * _ScreenSize.xy, reflPosNDC.z);
    float3 rayDir     = reflPosSS - rayOrigin;
    float3 rcpRayDir  = rcp(rayDir);
    int2   rayStep    = int2(rcpRayDir.x >= 0 ? 1 : 0,
                             rcpRayDir.y >= 0 ? 1 : 0);
    float3 raySign  = float3(rcpRayDir.x >= 0 ? 1 : -1,
                             rcpRayDir.y >= 0 ? 1 : -1,
                             rcpRayDir.z >= 0 ? 1 : -1);
    bool   rayTowardsEye  =  rcpRayDir.z >= 0;

    // Note that we don't need to store or read the perceptualRoughness value
    // if we mark stencil during the G-Buffer pass with pixels which should receive SSR,
    // and sample the color pyramid during the lighting pass.
    killRay = killRay || (reflPosSS.z <= 0);
    killRay = killRay || (dot(N, V) <= 0);
    killRay = killRay || (perceptualRoughness > _SsrRoughnessFadeEnd);
#ifndef SSR_TRACE_TOWARDS_EYE
    killRay = killRay || rayTowardsEye;
#endif

    if (killRay)
    {
        WriteDebugInfo(positionSS, -1);
        return;
    }

    // Extend and clip the end point to the frustum.
    float tMax;
    {
        // Shrink the frustum by half a texel for efficiency reasons.
        const float halfTexel = 0.5;

        float3 bounds;
        bounds.x = (rcpRayDir.x >= 0) ? _ScreenSize.x - halfTexel : halfTexel;
        bounds.y = (rcpRayDir.y >= 0) ? _ScreenSize.y - halfTexel : halfTexel;
        // If we do not want to intersect the skybox, it is more efficient to not trace too far.
        float maxDepth = (_SsrReflectsSky != 0) ? -0.00000024 : 0.00000024; // 2^-22
        bounds.z = (rcpRayDir.z >= 0) ? 1 : maxDepth;

        float3 dist = bounds * rcpRayDir - (rayOrigin * rcpRayDir);
        tMax = Min3(dist.x, dist.y, dist.z);
    }

    // Clamp the MIP level to give the compiler more information to optimize.
    const int maxMipLevel = min(_SsrDepthPyramidMaxMip, 14);

    // Start ray marching from the next texel to avoid self-intersections.
    float t;
    {
        // 'rayOrigin' is the exact texel center.
        float2 dist = abs(0.5 * rcpRayDir.xy);
        t = min(dist.x, dist.y);
    }

    float3 rayPos;

    int  mipLevel  = 0;
    int  iterCount = 0;
    bool hit       = false;
    bool miss      = false;
    bool belowMip0 = false; // This value is set prior to entering the cell

    while (!(hit || miss) && (t <= tMax) && (iterCount < _SsrIterLimit))
    {
        rayPos = rayOrigin + t * rayDir;

        // Ray position often ends up on the edge. To determine (and look up) the right cell,
        // we need to bias the position by a small epsilon in the direction of the ray.
        float2 sgnEdgeDist = round(rayPos.xy) - rayPos.xy;
        float2 satEdgeDist = clamp(raySign.xy * sgnEdgeDist + SSR_TRACE_EPS, 0, SSR_TRACE_EPS);
        rayPos.xy += raySign.xy * satEdgeDist;

        int2 mipCoord  = (int2)rayPos.xy >> mipLevel;
        // int2 mipOffset = _DepthPyramidMipLevelOffsets[mipLevel];
        // Bounds define 4 faces of a cube:
        // 2 walls in front of the ray, and a floor and a base below it.
        float4 bounds;

        bounds.xy = (mipCoord + rayStep) << mipLevel;
        bounds.z  = LOAD_TEXTURE2D_LOD(depthTexture, mipCoord, mipLevel).r;

        // We define the depth of the base as the depth value as:
        // b = DeviceDepth((1 + thickness) * LinearDepth(d))
        // b = ((f - n) * d + n * (1 - (1 + thickness))) / ((f - n) * (1 + thickness))
        // b = ((f - n) * d - n * thickness) / ((f - n) * (1 + thickness))
        // b = d / (1 + thickness) - n / (f - n) * (thickness / (1 + thickness))
        // b = d * k_s + k_b
        bounds.w = bounds.z * _SsrThicknessScale + _SsrThicknessBias;

        float4 dist      = bounds * rcpRayDir.xyzz - (rayOrigin.xyzz * rcpRayDir.xyzz);
        float  distWall  = min(dist.x, dist.y);
        float  distFloor = dist.z;
        float  distBase  = dist.w;

        // Note: 'rayPos' given by 't' can correspond to one of several depth values:
        // - above or exactly on the floor
        // - inside the floor (between the floor and the base)
        // - below the base
    #if 0
        bool belowFloor  = (raySign.z * (t - distFloor)) <  0;
        bool aboveBase   = (raySign.z * (t - distBase )) >= 0;
    #else
        bool belowFloor  = rayPos.z  < bounds.z;
        bool aboveBase   = rayPos.z >= bounds.w;
    #endif
        bool insideFloor = belowFloor && aboveBase;
        bool hitFloor    = (t <= distFloor) && (distFloor <= distWall);

        // Game rules:
        // * if the closest intersection is with the wall of the cell, switch to the coarser MIP, and advance the ray.
        // * if the closest intersection is with the heightmap below,  switch to the finer   MIP, and advance the ray.
        // * if the closest intersection is with the heightmap above,  switch to the finer   MIP, and do NOT advance the ray.
        // Victory conditions:
        // * See below. Do NOT reorder the statements!

    #ifdef SSR_TRACE_BEHIND_OBJECTS
        miss      = belowMip0 && insideFloor;
    #else
        miss      = belowMip0;
    #endif
        hit       = (mipLevel == 0) && (hitFloor || insideFloor);
        belowMip0 = (mipLevel == 0) && belowFloor;

        // 'distFloor' can be smaller than the current distance 't'.
        // We can also safely ignore 'distBase'.
        // If we hit the floor, it's always safe to jump there.
        // If we are at (mipLevel != 0) and we are below the floor, we should not move.
        t = hitFloor ? distFloor : (((mipLevel != 0) && belowFloor) ? t : distWall);
        rayPos.z = bounds.z; // Retain the depth of the potential intersection

        // Warning: both rays towards the eye, and tracing behind objects has linear
        // rather than logarithmic complexity! This is due to the fact that we only store
        // the maximum value of depth, and not the min-max.
        mipLevel += (hitFloor || belowFloor || rayTowardsEye) ? -1 : 1;
        mipLevel  = clamp(mipLevel, 0, maxMipLevel);

        // mipLevel = 0;

        iterCount++;
    }

    // Treat intersections with the sky as misses.
    miss = miss || ((_SsrReflectsSky == 0) && (rayPos.z == 0));
    hit  = hit && !miss;

    if (hit)
    {
        // Note that we are using 'rayPos' from the penultimate iteration, rather than
        // recompute it using the last value of 't', which would result in an overshoot.
        // It also needs to be precisely at the center of the pixel to avoid artifacts.
        float2 hitPositionNDC = floor(rayPos.xy) * _ScreenSize.zw + (0.5 * _ScreenSize.zw); // Should we precompute the half-texel bias? We seem to use it a lot.
        _SsrHitPointTexture[positionSS] = hitPositionNDC.xy;
    }

    // If we do not hit anything, 'rayPos.xy' provides an indication where we stopped the search.
    WriteDebugInfo(positionSS, float4(rayPos.xy, iterCount, hit ? 1 : 0));
}

#elif defined(SSR_REPROJECT)

[numthreads(8, 8, 1)]
void ScreenSpaceReflectionsReprojection(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    UNITY_XR_ASSIGN_VIEW_INDEX(dispatchThreadId.z);

    const uint2 positionSS0 = dispatchThreadId.xy;

    float3 N;
    float perceptualRoughness;
    GetNormalAndPerceptualRoughness(positionSS0, N, perceptualRoughness);

    // Compute the actual roughness
    float roughness = PerceptualRoughnessToRoughness(perceptualRoughness);
    roughness = clamp(roughness, MIN_GGX_ROUGHNESS, MAX_GGX_ROUGHNESS);

    float2 hitPositionNDC = LOAD_TEXTURE2D_X(_SsrHitPointTexture, positionSS0).xy;

    if (max(hitPositionNDC.x, hitPositionNDC.y) == 0)
    {
        // Miss.
        return;
    }

#ifdef DEPTH_SOURCE_NOT_FROM_MIP_CHAIN
    float  depthOrigin = LOAD_TEXTURE2D_X(_DepthTexture, positionSS0.xy).r;
#else
    float  depthOrigin = LOAD_TEXTURE2D_X(_CameraDepthTexture, positionSS0.xy).r;
#endif

    PositionInputs posInputOrigin = GetPositionInput(positionSS0.xy, _ScreenSize.zw, depthOrigin, UNITY_MATRIX_I_VP, UNITY_MATRIX_V, uint2(8, 8));
    float3 originWS = posInputOrigin.positionWS + _WorldSpaceCameraPos;

    // TODO: this texture is sparse (mostly black). Can we avoid reading every texel? How about using Hi-S?
    float2 motionVectorNDC;
    DecodeMotionVector(SAMPLE_TEXTURE2D_X_LOD(_CameraMotionVectorsTexture, s_linear_clamp_sampler, min(hitPositionNDC, 1.0f - 0.5f * _ScreenSize.zw) * _RTHandleScale.xy, 0), motionVectorNDC);
    float2 prevFrameNDC = hitPositionNDC - motionVectorNDC;
    float2 prevFrameUV = prevFrameNDC * _ColorPyramidUvScaleAndLimitPrevFrame.xy;

    // TODO: filtering is quite awful. Needs to be non-Gaussian, bilateral and anisotropic.
    float  mipLevel = lerp(0, _SsrColorPyramidMaxMip, perceptualRoughness);

    float2 diffLimit = _ColorPyramidUvScaleAndLimitPrevFrame.xy - _ColorPyramidUvScaleAndLimitPrevFrame.zw;
    float2 diffLimitMipAdjusted = diffLimit * pow(2.0,1.5 + ceil(abs(mipLevel)));
    float2 limit = _ColorPyramidUvScaleAndLimitPrevFrame.xy - diffLimitMipAdjusted;
    if (any(prevFrameUV < float2(0.0,0.0)) || any(prevFrameUV > limit))
    {
        // Off-Screen.
        return;
    }
    float  opacity  = EdgeOfScreenFade(prevFrameNDC, _SsrEdgeFadeRcpLength)
                    * PerceptualRoughnessFade(perceptualRoughness, _SsrRoughnessFadeRcpLength, _SsrRoughnessFadeEndTimesRcpLength);

#ifdef SSR_APPROX
    // Note that the color pyramid uses it's own viewport scale, since it lives on the camera.
    float3 color    = SAMPLE_TEXTURE2D_X_LOD(_ColorPyramidTexture, s_trilinear_clamp_sampler, prevFrameUV, mipLevel).rgb;

    // Disable SSR for negative, infinite and NaN history values.
    uint3 intCol   = asuint(color);
    bool  isPosFin = Max3(intCol.r, intCol.g, intCol.b) < 0x7F800000;

    color   = isPosFin ? color   : 0;
    opacity = isPosFin ? opacity : 0;

    _SSRAccumTexture[COORD_TEXTURE2D_X(positionSS0)] = float4(color, 1.0f) * opacity;
#else
    float3 color = 0.0f;

    float4 accum = _SSRAccumTexture[COORD_TEXTURE2D_X(positionSS0)];

#define BLOCK_SAMPLE_RADIUS 1
    int samplesCount = 0;
    float4 outputs = 0.0f;
    float wAll = 0.0f;
    for (int y = -BLOCK_SAMPLE_RADIUS; y <= BLOCK_SAMPLE_RADIUS; ++y)
    {
        for (int x = -BLOCK_SAMPLE_RADIUS; x <= BLOCK_SAMPLE_RADIUS; ++x)
        {
            if (abs(x) == abs(y) && abs(x) == 1)
                continue;

            uint2 positionSS = uint2(int2(positionSS0) + int2(x, y));

            float3 color;
            float opacity;
            float weight;
            float2 hitData = GetSampleInfo(positionSS, color, weight, opacity);
            if (max(hitData.x, hitData.y) != 0.0f && opacity > 0.0f)
            {
                //// Note that the color pyramid uses it's own viewport scale, since it lives on the camera.
                // Disable SSR for negative, infinite and NaN history values.
                uint3 intCol   = asuint(color);
                bool  isPosFin = Max3(intCol.r, intCol.g, intCol.b) < 0x7F800000;

                float2 prevFrameUV = hitData * _ColorPyramidUvScaleAndLimitPrevFrame.xy;

                color   = isPosFin ? color : 0;

                outputs += weight * float4(color, 1.0f);
                wAll += weight;
            }
        }
    }
#undef BLOCK_SAMPLE_RADIUS

    if (wAll > 0.0f)
    {
        uint3 intCol = asuint(outputs.rgb);
        bool  isPosFin = Max3(intCol.r, intCol.g, intCol.b) < 0x7F800000;

        outputs.rgb = isPosFin ? outputs.rgb : 0;
        opacity     = isPosFin ? opacity : 0;
        wAll = isPosFin ? wAll : 0;

        _SSRAccumTexture[COORD_TEXTURE2D_X(positionSS0)] = opacity * outputs / wAll;
    }
#endif
}

#elif defined(SSR_ACCUMULATE)

//#define __PLAIN_COLOR
//#define __MULTIPLY_COLOR
#define __CHROMINANCE

float3 Colorize(float3 current, float3 color)
{
#ifdef __PLAIN_COLOR
    return color;
#elif defined(__MULTIPLY_COLOR)
    return color * current;
#else // __CHROMINANCE
    float3 tmp = color * current;
    return tmp / max(tmp.r, max(tmp.g, tmp.b));
#endif
}

[numthreads(8, 8, 1)]
void MAIN_ACC(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    UNITY_XR_ASSIGN_VIEW_INDEX(dispatchThreadId.z);
    uint2 positionSS = dispatchThreadId.xy;

    float3 N;
    float perceptualRoughness0;
    GetNormalAndPerceptualRoughness(positionSS, N, perceptualRoughness0);

    // Compute the actual roughness
    float roughness = PerceptualRoughnessToRoughness(perceptualRoughness0);
    roughness = clamp(roughness, MIN_GGX_ROUGHNESS, MAX_GGX_ROUGHNESS);

    float4 data0 = _SSRAccumTexture[COORD_TEXTURE2D_X(int2(positionSS))];

    float2 hitPositionNDC = LOAD_TEXTURE2D_X(_SsrHitPointTexture, positionSS).xy;

    // Approximate the footprint based on the hit normal
    float2 hitSS = (hitPositionNDC.xy - (0.5 * _ColorPyramidUvScaleAndLimitPrevFrame.zw)) / _ColorPyramidUvScaleAndLimitPrevFrame.zw;

    NormalData hitNormalData;
    DecodeFromNormalBuffer(hitSS, hitNormalData);
    float3 hitN = hitNormalData.normalWS;

    float2 prevHistoryScale = _RTHandleScaleHistory.zw / _RTHandleScaleHistory.xy;
    float4 original = _SSRAccumTexture[COORD_TEXTURE2D_X(positionSS * _RTHandleScaleHistory.zw / _RTHandleScaleHistory.xy)];

    float4 previous = _SsrAccumPrev[COORD_TEXTURE2D_X(positionSS * prevHistoryScale + 0.5f / prevHistoryScale)];

    float2 motionVectorNDC;
    DecodeMotionVector(SAMPLE_TEXTURE2D_X_LOD(_CameraMotionVectorsTexture, s_linear_clamp_sampler, min(hitPositionNDC.xy, 1.0f - 0.5f * _ScreenSize.zw) * _RTHandleScale.xy, 0), motionVectorNDC);

    float2 motionVectorCenterNDC;
    float2 positionNDC = positionSS * _ScreenSize.zw + (0.5 * _ScreenSize.zw);
    DecodeMotionVector(SAMPLE_TEXTURE2D_X_LOD(_CameraMotionVectorsTexture, s_linear_clamp_sampler, min(positionNDC, 1.0f - 0.5f * _ScreenSize.zw) * _RTHandleScale.xy, 0), motionVectorCenterNDC);

#ifdef WORLD_SPEED_REJECTION
#ifdef DEPTH_SOURCE_NOT_FROM_MIP_CHAIN
#if USE_SPEED_SURFACE
    float  deviceDepthSrc = LOAD_TEXTURE2D_X(_DepthTexture, positionSS).r;
#endif
#if USE_SPEED_TARGET
    float  deviceDepthDst = LOAD_TEXTURE2D_X(_DepthTexture, hitSS).r;
#endif
#else
#if USE_SPEED_SURFACE
    float  deviceDepthSrc = LOAD_TEXTURE2D_X(_CameraDepthTexture, positionSS).r;
#endif
#if USE_SPEED_TARGET
    float  deviceDepthDst = LOAD_TEXTURE2D_X(_CameraDepthTexture, hitSS).r;
#endif
#endif

#if USE_SPEED_SURFACE
    float4 srcMotion = float4(motionVectorCenterNDC.xy, deviceDepthSrc, 0.0f);
    float4 worldSrcMotion = mul(UNITY_MATRIX_I_VP, srcMotion);
    worldSrcMotion /= worldSrcMotion.w;
#endif

#if USE_SPEED_TARGET
    float4 dstMotion = float4(motionVectorNDC.xy, deviceDepthDst, 0.0f);
    float4 worldDstMotion = mul(UNITY_MATRIX_I_VP, dstMotion);
    worldDstMotion /= worldDstMotion.w;
#endif

#if USE_SPEED_SURFACE == 1 && USE_SPEED_TARGET == 0
    float speedDiff = length(worldSrcMotion.xyz);
#elif USE_SPEED_SURFACE == 0 && USE_SPEED_TARGET == 1
    float speedDiff = length(worldDstMotion.xyz);
#elif USE_SPEED_SURFACE == 1 && USE_SPEED_TARGET == 1
    float speedDiff = length(worldSrcMotion.xyz + worldDstMotion.xyz);
#else
    #error need at least one source of motion vector
#endif

    float usedSpeed = saturate(Normalize01(clamp(speedDiff, 0.0f, _SsrPRBSpeedRejectionScalerFactor), 0.0f, _SsrPRBSpeedRejectionScalerFactor));

#ifdef SSR_SMOOTH_SPEED_REJECTION
    float coefExpAvg = lerp(_SsrAccumulationAmount, 1.0f, saturate(usedSpeed * _SsrPBRSpeedRejection));
#else
    float coefExpAvg = usedSpeed >= _SsrPBRSpeedRejection ? 1.0f : _SsrAccumulationAmount;
#endif
#else

#if USE_SPEED_SURFACE == 1 && USE_SPEED_TARGET == 0
    float speedUsedLocally = length(motionVectorCenterNDC);
#elif USE_SPEED_SURFACE == 0 && USE_SPEED_TARGET == 1
    float speedUsedLocally = length(motionVectorNDC);
#elif USE_SPEED_SURFACE == 1 && USE_SPEED_TARGET == 1
    float speedSrc = length(motionVectorCenterNDC);
    float speedDst = length(motionVectorNDC);
    float speedUsedLocally = speedSrc + speedDst;
#else
    #error need at least one source of motion vector
#endif

    // 128 is an arbitrary value used historical
    // 0.5f is the default for the parameter 'Speed Rejection': ScreenSpaceReflection.cs 'speedRejectionParam'
    // 0.2f is the default for the parameter 'Speed Rejection Scaler Factor': ScreenSpaceReflection.cs 'speedRejectionScalerFactor'
    float speed = saturate(speedUsedLocally * (128.0f / (0.5f * 0.2f)) * _SsrPBRSpeedRejection);

    float coefExpAvg = lerp(_SsrAccumulationAmount, 1.0f, speed);
#endif

    float4 result = lerp(previous, original, coefExpAvg);

#ifdef REJECT_DEBUG_COLOR
#ifdef WORLD_SPEED_REJECTION
    #ifdef SSR_SMOOTH_SPEED_REJECTION
        result.rgb = Colorize(result.rgb, HsvToRgb(lerp(float3(0.33333333333333333333333333f, 1.0f, 1.0f), float3(0.0f, 1.0f, 1.0f), saturate(coefExpAvg))));
    #else
        if (usedSpeed >= _SsrPBRSpeedRejection)
            result.rgb = Colorize(result.rgb, float3(1.0f, 0.0f, 0.0f));
        else
            result.rgb = Colorize(result.rgb, float3(0.0f, 1.0f, 0.0f));
    #endif
#else
    result.rgb = Colorize(result.rgb, HsvToRgb(lerp(float3(0.33333333333333333333333333f, 1.0f, 1.0f), float3(0.0f, 1.0f, 1.0f), saturate(coefExpAvg))));
#endif
#endif

    uint3 intCol = asuint(result.rgb);
    bool  isPosFin = Max3(intCol.r, intCol.g, intCol.b) < 0x7F800000;

    result.rgb = isPosFin ? result.rgb : 0;
    result.w = isPosFin ? result.w : 0;

    _SsrLightingTextureRW[COORD_TEXTURE2D_X(positionSS)] = result;
    _SSRAccumTexture[COORD_TEXTURE2D_X(positionSS)] = result;
}

#endif












