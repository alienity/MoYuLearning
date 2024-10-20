#include "d3d12.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"

cbuffer RootConstants : register(b0, space0)
{
    uint perFrameBufferIndex;
    uint depthBufferIndex;
    uint blueNoiseIndex;
    uint volume3DIndex;
};

SamplerState defaultSampler : register(s10);
SamplerState depthSampler : register(s11);
SamplerComparisonState shadowmapSampler : register(s12);

/**
 * Returns the cascade index for this fragment (between 0 and CONFIG_MAX_SHADOW_CASCADES - 1).
 */
void getShadowCascade(
    float4x4 viewFromWorldMatrix, float3 worldPosition, uint4 shadow_bounds, uint cascadeCount, 
    out uint cascadeLevel, out float2 shadow_bound_offset) 
{
    float2 posInView = mulMat4x4Float3(viewFromWorldMatrix, worldPosition).xy;
    float x = max(abs(posInView.x), abs(posInView.y));
    uint4 greaterZ = uint4(step(shadow_bounds * 0.9f * 0.5f, float4(x, x, x, x)));
    cascadeLevel = greaterZ.x + greaterZ.y + greaterZ.z + greaterZ.w;
    shadow_bound_offset = step(1, cascadeLevel & uint2(0x1, 0x2)) * 0.5;
}

float sampleDepth(Texture2D<float> shadowmap, const float2 offseted_uv, const float depth)
{
    float tempfShadow = shadowmap.SampleCmpLevelZero(shadowmapSampler, offseted_uv, depth);
    return tempfShadow;
}

float shadowSample_PCF_Low(Texture2D<float> shadowmap, 
    const float4x4 light_proj_view, uint2 shadowmap_size, float2 uv_offset, const float uv_scale, const float3 positionWS)
{
    float4 fragPosLightSpace = mul(light_proj_view, float4(positionWS, 1.0f));

    // perform perspective divide
    float3 shadowPosition = fragPosLightSpace.xyz / fragPosLightSpace.w;

    if(max(abs(shadowPosition.x), abs(shadowPosition.y)) > 1)
        return 0.0f;

    shadowPosition.xy = shadowPosition.xy * 0.5 + 0.5;
    shadowPosition.y = 1 - shadowPosition.y;

    float2 texelSize = float2(1.0f, 1.0f) / shadowmap_size;

    //  Castaño, 2013, "Shadow Mapping Summary Part 1"
    float depth = shadowPosition.z;

    return sampleDepth(shadowmap, (shadowPosition.xy * uv_scale + uv_offset), depth);

    float2 offset = float2(0.5, 0.5);
    float2 uv = (shadowPosition.xy * shadowmap_size) + offset;
    float2 base = (floor(uv) - offset) * texelSize;
    float2 st = frac(uv);

    float2 uw = float2(3.0 - 2.0 * st.x, 1.0 + 2.0 * st.x);
    float2 vw = float2(3.0 - 2.0 * st.y, 1.0 + 2.0 * st.y);

    float2 u = float2((2.0 - st.x) / uw.x - 1.0, st.x / uw.y + 1.0);
    float2 v = float2((2.0 - st.y) / vw.x - 1.0, st.y / vw.y + 1.0);

    u *= texelSize.x;
    v *= texelSize.y;

    float sum = 0.0;
    sum += uw.x * vw.x * sampleDepth(shadowmap, (base + float2(u.x, v.x)) * uv_scale + uv_offset, depth);
    sum += uw.y * vw.x * sampleDepth(shadowmap, (base + float2(u.y, v.x)) * uv_scale + uv_offset, depth);
    sum += uw.x * vw.y * sampleDepth(shadowmap, (base + float2(u.x, v.y)) * uv_scale + uv_offset, depth);
    sum += uw.y * vw.y * sampleDepth(shadowmap, (base + float2(u.y, v.y)) * uv_scale + uv_offset, depth);
    return sum * (1.0 / 16.0);
}

float cloudShadowSample(const Texture2D<float> cloudShadowmap, const float3 positionWS, const float2 volumebounds)
{
    float2 coords = (positionWS.xz / volumebounds.xy) + 0.5.xx;
    float attenuation = cloudShadowmap.SampleLevel(defaultSampler, coords, 0).r;
    return 1 - attenuation;
}

//-----------------------------------------------------------------------------------------
// GetLightAttenuation
//-----------------------------------------------------------------------------------------
float getLightAttenuation(const Texture2D<float> shadowmap, const Texture2D<float> cloudShadowmap, const float2 cloud_bounds,
    const float4x4 light_view_matrix, const float4x4 light_proj_view[4], 
    const float4 shadow_bounds, uint2 shadowmap_size, uint cascadeCount, const float3 positionWS)
{
    uint cascadeLevel;
    float2 shadow_bound_offset;
    getShadowCascade(light_view_matrix, positionWS, shadow_bounds, cascadeCount, cascadeLevel, shadow_bound_offset);

    if(cascadeLevel == 4)
        return 0.0f;
    
    float uv_scale = 1.0f / log2(cascadeCount);

    float shadowWeights = shadowSample_PCF_Low(
        shadowmap, light_proj_view[cascadeLevel], shadowmap_size, shadow_bound_offset, uv_scale, positionWS);

    float cloudShadowWeights = cloudShadowSample(cloudShadowmap, positionWS, cloud_bounds);
    
    return shadowWeights * cloudShadowWeights;
}

//-----------------------------------------------------------------------------------------
// ApplyHeightFog
//-----------------------------------------------------------------------------------------
void applyHeightFog(float3 positionWS, float groundLevel, float heightScale, inout float density)
{
    density *= exp(-(positionWS.y + groundLevel) * heightScale);
}

//-----------------------------------------------------------------------------------------
// GetDensity
//-----------------------------------------------------------------------------------------
float getVolumeDensity(float3 positionWS, float groundLevel, float heightScale)
{
    float density = 1;

    // float noise = tex3D(_NoiseTexture, frac(positionWS * _NoiseData.x + float3(_Time.y * _NoiseVelocity.x, 0, _Time.y * _NoiseVelocity.y)));
    // noise = saturate(noise - _NoiseData.z) * _NoiseData.y;
    // density = saturate(noise);

    applyHeightFog(positionWS, groundLevel, heightScale, density);

    return density;
}        

//-----------------------------------------------------------------------------------------
// MieScattering
//-----------------------------------------------------------------------------------------
float CornetteShanksPhasePartConstant(float anisotropy)
{
    float g = anisotropy;

    return (3 / (8 * PI)) * (1 - g * g) / (2 + g * g);
}

// Similar to the RayleighPhaseFunction.
float CornetteShanksPhasePartSymmetrical(float cosTheta)
{
    float h = 1 + cosTheta * cosTheta;
    return h;
}

float CornetteShanksPhasePartAsymmetrical(float anisotropy, float cosTheta)
{
    float g = anisotropy;
    float x = 1 + g * g - 2 * g * cosTheta;
    float f = rsqrt(max(x, FLT_EPS)); // x^(-1/2)
    return f * f * f;                 // x^(-3/2)
}

float CornetteShanksPhasePartVarying(float anisotropy, float cosTheta)
{
    return CornetteShanksPhasePartSymmetrical(cosTheta) *
           CornetteShanksPhasePartAsymmetrical(anisotropy, cosTheta); // h * x^(-3/2)
}

// A better approximation of the Mie phase function.
// Ref: Henyey-Greenstein and Mie phase functions in Monte Carlo radiative transfer computations
float CornetteShanksPhaseFunction(float anisotropy, float cosTheta)
{
    return CornetteShanksPhasePartConstant(anisotropy) *
           CornetteShanksPhasePartVarying(anisotropy, cosTheta);
}

float mieScattering(float cosAngle, float g)
{
    return CornetteShanksPhaseFunction(g, cosAngle);
}

//-----------------------------------------------------------------------------------------
// RayMarch
//-----------------------------------------------------------------------------------------
void rayMarch(FrameUniforms frameUniform, Texture2D<float> shadowmapTex, Texture2D<float> cloudShadowmap, Texture2D<float> depthBuffer,
    RWTexture3D<float4> volumeResult, uint2 dtid, float rayOffset, float3 rayStart, float3 rayTarget, float rayLength)
{
    DirectionalLightStruct _directionLight = frameUniform.directionalLight;
    VolumeLightUniform _volumeLightUniform = frameUniform.volumeLightUniform;
    DirectionalLightShadowmap _dirShadowmap = _directionLight.directionalLightShadowmap;

    const float3 lightColor = _directionLight.lightColorIntensity.rgb;
    const float3 lightDir = _directionLight.lightDirection;
    const float mieG = _volumeLightUniform.mieG;
    const float scattering_coef = _volumeLightUniform.scattering_coef;
    const float extinction_coef = _volumeLightUniform.extinction_coef;
    const int stepCount = _volumeLightUniform.sampleCount;
    const float ground_level = _volumeLightUniform.ground_level;
    const float height_scale = _volumeLightUniform.height_scale;
    const float minStepSize = _volumeLightUniform.minStepSize;
    const float4x4 light_view_matrix = _dirShadowmap.light_view_matrix;
    const float4x4 light_proj_view[4] = _dirShadowmap.light_proj_view; 
    const float4 shadow_bounds = _dirShadowmap.shadow_bounds;
    const uint2 shadowmap_size = _dirShadowmap.shadowmap_size;
    const uint cascadeCount = _dirShadowmap.cascadeCount;
    
    const float2 cloudbounds = float2(frameUniform.volumeCloudUniform.cloud_shadowmap_bounds);

    // float2 interleavedPos = (fmod(floor(screenPos.xy), 8.0));
    // float offset = tex2D(blueNoiseTex, interleavedPos / 8.0 + float2(0.5 / 8.0, 0.5 / 8.0)).r;

    float3 rayDir = rayTarget - rayStart;
    
    float rayDepth = length(rayDir);
    rayDir = rayDir / rayDepth;

    int step1Count = stepCount * 0.5f;
    int step2Count = stepCount - step1Count;

    float step1StepSize = minStepSize;
    float step1Distance = step1StepSize * step1Count;
    float step2StepSize = (rayLength - step1Distance) / step2Count;

    // float stepSize = rayLength / stepCount;
    // float3 stepDir = rayDir * step1StepSize;

    float3 currentPositionWS = rayStart + rayOffset * rayDir * step1StepSize;

    float4 vlight = 0;

    float extinction = 0;
    float cosAngle = dot(lightDir.xyz, -rayDir);

    float miePhase = mieScattering(cosAngle, mieG);

    [loop]
    for (int i = 0; i < stepCount; ++i)
    {
        float stepSize = step1StepSize;
        if(i > step1Count)
            stepSize = step2StepSize;

        float light_attenuation = getLightAttenuation(
            shadowmapTex, cloudShadowmap, cloudbounds, 
            light_view_matrix, light_proj_view, shadow_bounds, shadowmap_size, cascadeCount, currentPositionWS);
        float density = getVolumeDensity(currentPositionWS, ground_level, height_scale);

        float scattering = scattering_coef * stepSize * density;
        extinction += extinction_coef * stepSize * density;// +scattering;

        float transparency = exp(-extinction);

        float3 light = transparency * lightColor * light_attenuation * miePhase * scattering;

        light = light * step(i, rayDepth);
        // if (rayDepth < i)
        // {
        //     light = float3(0, 0, 0);
        // }

        vlight.rgb = max(vlight.rgb + light, 0);
        // use "proper" out-scattering/absorption for dir light 
        vlight.w = exp(-extinction);

        volumeResult[uint3(dtid.xy, i)] = vlight;
        // float3 lightViewPos = mul(light_view_matrix, float4(currentPositionWS.xyz, 1.0f)).xyz;
        // volumeResult[uint3(dtid.xy, i)] = float4(rayTarget.xyz, 1);

        currentPositionWS += rayDir * stepSize;
    }
}

float3 getWorldPosition(float3 clipPos, float4x4 clipToWorldMatrix)
{
    float4 worldPos = mul(clipToWorldMatrix, float4(clipPos, 1.0f));
    float3 outPos = worldPos.xyz / worldPos.w;
    return outPos;
}

float3 getViewPosition(float3 clipPos, float4x4 clipToViewMatrix)
{
    float4 viewPos = mul(clipToViewMatrix, float4(clipPos, 1.0f));
    float3 outPos = viewPos.xyz / viewPos.w;
    return outPos;
}

// https://www.scratchapixel.com/lessons/3d-basic-rendering/volume-rendering-for-developers/ray-marching-get-it-right.html
// https://github.com/scratchapixel/code

[numthreads( 8, 8, 1 )]
void CSMain( uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID )
{
    ConstantBuffer<FrameUniforms> g_FrameUniform = ResourceDescriptorHeap[perFrameBufferIndex];
    Texture2D<float3> blueNoiseTex = ResourceDescriptorHeap[blueNoiseIndex];
    Texture2D<float> depthBuffer = ResourceDescriptorHeap[depthBufferIndex];
    Texture2D<float> dirShadowmap = ResourceDescriptorHeap[g_FrameUniform.directionalLight.directionalLightShadowmap.shadowmap_srv_index];
    Texture2D<float> cloudShadowmap = ResourceDescriptorHeap[g_FrameUniform.volumeCloudUniform.cloud_shadowmap_srv_index];
    RWTexture3D<float4> volume3DResult = ResourceDescriptorHeap[volume3DIndex];

    int width, height, depthLength;
    volume3DResult.GetDimensions(width, height, depthLength);

    if (DTid.x > width || DTid.y > height)
        return;

    float2 screenPos = DTid.xy + float2(0.5, 0.5);

    int mipOffset = g_FrameUniform.volumeLightUniform.downscaleMip;

    float2 uv = screenPos / float2(width, height);
    float depth = depthBuffer.SampleLevel(depthSampler, uv, mipOffset).r;

    float4x4 clipToViewMatrix = g_FrameUniform.cameraUniform.curFrameUniform.viewFromClipMatrix;
    float4x4 viewToWorldMatrix = g_FrameUniform.cameraUniform.curFrameUniform.worldFromViewMatrix;
    float4x4 clipToWorldMatrix = g_FrameUniform.cameraUniform.curFrameUniform.worldFromClipMatrix;

    float curTime = g_FrameUniform.baseUniform.time;

    float2 interleavedPos = (fmod(floor(screenPos.xy), 8.0));
    float offset = blueNoiseTex.SampleLevel(defaultSampler, interleavedPos / 8.0 + float2(0.5 / 8.0, 0.5 / 8.0), 0).r;

    offset = offset * (0.5f+0.5f*sin(curTime*20));

    // 只有在使用1-uv.y的时候，还原出来的世界空间顶点才是正确的，肯定是我前面哪里写错了
    float3 targetPosition = getWorldPosition(float3(uv.x*2-1, (1-uv.y)*2-1, depth), clipToWorldMatrix);
    float3 rayStart = getWorldPosition(float3(uv.x*2-1, (1-uv.y)*2-1, 1.0f), clipToWorldMatrix);

    float rayLength = g_FrameUniform.volumeLightUniform.maxRayLength;
 
    // [loop]
    // for (int i = 0; i < g_FrameUniform.volumeLightUniform.sampleCount; ++i)
    // {
    //     float of = depth - finalPosition.z;
    //     volume3DResult[uint3(DTid.xy, i)] = float4(targetPosition.xyz, 1);
    //     // volume3DResult[uint3(DTid.xy, i)] = float4(finalPosition.z, finalPosition.z, finalPosition.z, 1);
    //     // volume3DResult[uint3(DTid.xy, i)] = float4(targetPosition.z, targetPosition.z, targetPosition.z, 1);
    //     // volume3DResult[uint3(DTid.xy, i)] = float4(of, of, of, 1);
    // }

    rayMarch(g_FrameUniform, dirShadowmap, cloudShadowmap, depthBuffer, volume3DResult, DTid.xy, offset, rayStart, targetPosition, rayLength);
}
