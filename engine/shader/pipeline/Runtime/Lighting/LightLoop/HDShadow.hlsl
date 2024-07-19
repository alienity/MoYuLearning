#ifndef LIGHTLOOP_HD_SHADOW_HLSL
#define LIGHTLOOP_HD_SHADOW_HLSL

#define SHADOW_OPTIMIZE_REGISTER_USAGE 1

#ifndef SHADOW_AUTO_FLIP_NORMAL   // If (NdotL < 0), we flip the normal to correctly bias lit back-faces (used for transmission)
#define SHADOW_AUTO_FLIP_NORMAL 1 // Externally define as 0 to disable
#endif

#ifndef SHADOW_USE_DEPTH_BIAS     // Enable clip space z biasing
#define SHADOW_USE_DEPTH_BIAS   1 // Externally define as 0 to disable
#endif

#if SHADOW_OPTIMIZE_REGISTER_USAGE == 1
#   pragma warning( disable : 3557 ) // loop only executes for 1 iteration(s)
#endif

# include "../../Lighting/Shadow/HDShadowContext.hlsl"

// Normal light loop is guaranteed to be scalar, but not always we sample shadows with said guarantee. In those cases we define SHADOW_DATA_NOT_GUARANTEED_SCALAR to skip the forcing into scalar.
#define FORCE_SHADOW_SCALAR_READ !defined(SHADOW_DATA_NOT_GUARANTEED_SCALAR) && !defined(SHADER_API_XBOXONE) && !defined(SHADER_API_GAMECORE) && (defined(PLATFORM_SUPPORTS_WAVE_INTRINSICS) && !defined(LIGHTLOOP_DISABLE_TILE_AND_CLUSTER))

// normalWS is the vertex normal if available or shading normal use to bias the shadow position
float GetDirectionalShadowAttenuation(
    inout HDShadowContext shadowContext,
    FrameUniforms frameUniform, RenderDataPerDraw renderData, PropertiesPerMaterial matProperties, SamplerStruct samplerStruct,
    float2 positionSS, float3 positionWS, float3 normalWS, int shadowDataIndex, float3 L)
{
#if SHADOW_AUTO_FLIP_NORMAL
    normalWS *= FastSign(dot(normalWS, L));
#endif
#if defined(SHADOW_ULTRA_LOW) || defined(SHADOW_LOW) || defined(SHADOW_MEDIUM)
    return EvalShadow_CascadedDepth_Dither(shadowContext, samplerStruct.SLinearClampCompareSampler, positionSS, positionWS, normalWS, shadowDataIndex, L);
#else
    return EvalShadow_CascadedDepth_Blend(shadowContext, _ShadowmapCascadeAtlas, s_linear_clamp_compare_sampler, positionSS, positionWS, normalWS, shadowDataIndex, L);
#endif
}

float GetPunctualShadowAttenuation(SamplerStruct samplerStruct, HDShadowContext shadowContext, 
    float2 positionSS, float3 positionWS, float3 normalWS, int shadowDataIndex, float3 L, float L_dist, bool pointLight, bool perspecive)
{
#if SHADOW_AUTO_FLIP_NORMAL
    normalWS *= FastSign(dot(normalWS, L));
#endif

    // Note: Here we assume that all the shadow map cube faces have been added contiguously in the buffer to retreive the shadow information
    HDShadowData sd = shadowContext.shadowDatas[shadowDataIndex];

    return EvalShadow_PunctualDepth(sd, samplerStruct.SLinearClampCompareSampler, positionSS, positionWS, normalWS, L, L_dist, perspecive);
}

float GetPunctualShadowClosestDistance(HDShadowContext shadowContext, SamplerState sampl, float3 positionWS, int shadowDataIndex, float3 L, float3 lightPositionWS, bool pointLight, bool perspective)
{
#if FORCE_SHADOW_SCALAR_READ
    shadowDataIndex = WaveReadLaneFirst(shadowDataIndex);
#endif

    // Note: Here we assume that all the shadow map cube faces have been added contiguously in the buffer to retreive the shadow information
    // TODO: if on the light type to retrieve the good shadow data
    HDShadowData sd = shadowContext.shadowDatas[shadowDataIndex];

    /*
    if (pointLight)
    {
        sd.shadowToWorld = shadowContext.shadowDatas[shadowDataIndex + CubeMapFaceID(-L)].shadowToWorld;
        sd.atlasOffset = shadowContext.shadowDatas[shadowDataIndex + CubeMapFaceID(-L)].atlasOffset;
        sd.rot0 = shadowContext.shadowDatas[shadowDataIndex + CubeMapFaceID(-L)].rot0;
        sd.rot1 = shadowContext.shadowDatas[shadowDataIndex + CubeMapFaceID(-L)].rot1;
        sd.rot2 = shadowContext.shadowDatas[shadowDataIndex + CubeMapFaceID(-L)].rot2;
    }
    */

    Texture2D<float4> _ShadowmapAtlas = ResourceDescriptorHeap[sd.shadowmapIndex];
    return EvalShadow_SampleClosestDistance_Punctual(sd, _ShadowmapAtlas, sampl, positionWS, L, lightPositionWS, perspective);
}

// float GetRectAreaShadowAttenuation(HDShadowContext shadowContext, float2 positionSS, float3 positionWS, float3 normalWS, int shadowDataIndex, float3 L, float L_dist)
// {
//     // We need to disable the scalarization here on xbox due to bad code generated by FXC for the eye shader.
//     // This shouldn't have an enormous impact since with Area lights we are already exploded in VGPR by this point.
// #if FORCE_SHADOW_SCALAR_READ
//     shadowDataIndex = WaveReadLaneFirst(shadowDataIndex);
// #endif
//     HDShadowData sd = shadowContext.shadowDatas[shadowDataIndex];
//
//     Texture2D<float4> _ShadowmapAreaAtlas = ResourceDescriptorHeap[sd.shadowmapIndex];
//     return EvalShadow_AreaDepth(sd, _ShadowmapAreaAtlas, positionSS, positionWS, normalWS, L, L_dist, true);
// }

#endif // LIGHTLOOP_HD_SHADOW_HLSL
