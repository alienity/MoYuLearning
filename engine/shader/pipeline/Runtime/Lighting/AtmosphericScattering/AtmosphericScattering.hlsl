#ifndef UNITY_ATMOSPHERIC_SCATTERING_INCLUDED
#define UNITY_ATMOSPHERIC_SCATTERING_INCLUDED

#include "../../ShaderLibrary/Macros.hlsl"
#include "../../ShaderLibrary/VolumeRendering.hlsl"
#include "../../ShaderLibrary/Filtering.hlsl"
#include "../../ShaderLibrary/GeometricTools.hlsl"

#define FOGCOLORMODE_CONSTANT_COLOR (0)
#define FOGCOLORMODE_SKY_COLOR (1)

#include "../../ShaderLibrary/ShaderVariables.hlsl"
#include "../../Lighting/VolumetricLighting/VBuffer.hlsl"
// #include "../../../Runtime/Sky/PhysicallyBasedSkyCommon.hlsl"

#define _PlanetCenterPosition(_PlanetCenterRadius) _PlanetCenterRadius.xyz // camera relative
#define _GroundAlbedo(_GroundAlbedo_PlanetRadius) _GroundAlbedo_PlanetRadius.xyz
#define _PlanetUp(_PlanetUpAltitude) _PlanetUpAltitude.xyz
#define _CameraAltitude(_PlanetUpAltitude) _PlanetUpAltitude.w

#define _PlanetaryRadius(_PlanetCenterRadius) _PlanetCenterRadius.w

float3 GetFogColor(float3 _FogColor, float3 V, float fragDist)
{
    float3 color = _FogColor.rgb;

    // if (_FogColorMode == FOGCOLORMODE_SKY_COLOR)
    // {
    //     // Based on Uncharted 4 "Mip Sky Fog" trick: http://advances.realtimerendering.com/other/2016/naughty_dog/NaughtyDog_TechArt_Final.pdf
    //     float mipLevel = (1.0 - _MipFogMaxMip * saturate((fragDist - _MipFogNear) / (_MipFogFar - _MipFogNear))) * (ENVCONSTANTS_CONVOLUTION_MIP_COUNT - 1);
    //     // For the atmospheric scattering, we use the GGX convoluted version of the cubemap. That matches the of the idnex 0
    //     color *= SampleSkyTexture(-V, mipLevel, 0).rgb; // '_FogColor' is the tint
    // }

    return color;
}

// // All units in meters!
// // Assumes that there is NO sky occlusion along the ray AT ALL.
// // We evaluate atmospheric scattering for the sky and other celestial bodies
// // during the sky pass. The opaque atmospheric scattering pass applies atmospheric
// // scattering to all other opaque geometry.
// void EvaluatePbrAtmosphere(
//     FrameUniforms frameUniform,
//     VolumetricLightingUniform volumeLightUniform,
//     ShaderVariablesPhysicallyBasedSky shaderVariablesPhysicallyBasedSky,
//     float3 worldSpaceCameraPos, float3 V, float distAlongRay, bool renderSunDisk,
//     out float3 skyColor, out float3 skyOpacity)
// {
//     skyColor = skyOpacity = 0;
//
//     const float  R = _PlanetaryRadius(volumeLightUniform._PlanetCenterRadius);
//     const float2 n = float2(shaderVariablesPhysicallyBasedSky._AirDensityFalloff, shaderVariablesPhysicallyBasedSky._AerosolDensityFalloff);
//     const float2 H = float2(shaderVariablesPhysicallyBasedSky._AirScaleHeight, shaderVariablesPhysicallyBasedSky._AerosolScaleHeight);
//
//     // TODO: Not sure it's possible to precompute cam rel pos since variables
//     // in the two constant buffers may be set at a different frequency?
//     const float3 O     = worldSpaceCameraPos - _PlanetCenterPosition(volumeLightUniform._PlanetCenterRadius);
//     const float  tFrag = abs(distAlongRay); // Clear the "hit ground" flag
//
//     float3 N; float r; // These params correspond to the entry point
//     float  tEntry = IntersectAtmosphere(O, V, N, r).x;
//     float  tExit  = IntersectAtmosphere(O, V, N, r).y;
//
//     float NdotV  = dot(N, V);
//     float cosChi = -NdotV;
//     float cosHor = ComputeCosineOfHorizonAngle(r);
//
//     bool rayIntersectsAtmosphere = (tEntry >= 0);
//     bool lookAboveHorizon        = (cosChi >= cosHor);
//
//     // Our precomputed tables only contain information above ground.
//     // Being on or below ground still counts as outside.
//     // If it's outside the atmosphere, we only need one texture look-up.
//     bool hitGround = distAlongRay < 0;
//     bool rayEndsInsideAtmosphere = (tFrag < tExit) && !hitGround;
//
//     if (rayIntersectsAtmosphere)
//     {
//         float2 Z = R * n;
//         float r0 = r, cosChi0 = cosChi;
//
//         float r1 = 0, cosChi1 = 0;
//         float3 N1 = 0;
//
//         if (tFrag < tExit)
//         {
//             float3 P1 = O + tFrag * -V;
//
//             r1      = length(P1);
//             N1      = P1 * rcp(r1);
//             cosChi1 = dot(P1, -V) * rcp(r1);
//
//             // Potential swap.
//             cosChi0 = (cosChi1 >= 0) ? cosChi0 : -cosChi0;
//         }
//
//         float2 ch0, ch1 = 0;
//
//         {
//             float2 z0 = r0 * n;
//
//             ch0.x = RescaledChapmanFunction(z0.x, Z.x, cosChi0);
//             ch0.y = RescaledChapmanFunction(z0.y, Z.y, cosChi0);
//         }
//
//         if (tFrag < tExit)
//         {
//             float2 z1 = r1 * n;
//
//             ch1.x = ChapmanUpperApprox(z1.x, abs(cosChi1)) * exp(Z.x - z1.x);
//             ch1.y = ChapmanUpperApprox(z1.y, abs(cosChi1)) * exp(Z.y - z1.y);
//         }
//
//         // We may have swapped X and Y.
//         float2 ch = abs(ch0 - ch1);
//
//         float3 optDepth = ch.x * H.x * shaderVariablesPhysicallyBasedSky._AirSeaLevelExtinction.xyz
//                         + ch.y * H.y * shaderVariablesPhysicallyBasedSky._AerosolSeaLevelExtinction;
//
//         skyOpacity = 1 - TransmittanceFromOpticalDepth(optDepth); // from 'tEntry' to 'tFrag'
//
//
//         const int _DirectionalLightCount = 1;
//         for (uint i = 0; i < _DirectionalLightCount; i++)
//         {
//             DirectionalLightData light = frameUniform.lightDataUniform.directionalLightData;
//             // DirectionalLightData light = _DirectionalLightDatas[i];
//
//             // Use scalar or integer cores (more efficient).
//             bool interactsWithSky = asint(light.distanceFromCamera) >= 0;
//
//             if (!interactsWithSky) continue;
//
//             float3 L = -light.forward.xyz;
//
//             // The sun disk hack causes some issues when applied to nearby geometry, so don't do that.
//             if (renderSunDisk && asint(light.angularDiameter) != 0 && light.distanceFromCamera <= tFrag)
//             {
//                 float c = dot(L, -V);
//
//                 if (-0.99999 < c && c < 0.99999)
//                 {
//                     float alpha = 0.5 * light.angularDiameter;
//                     float beta  = acos(c);
//                     float gamma = min(alpha, beta);
//
//                     // Make sure that if (beta = Pi), no rotation is performed.
//                     gamma *= (PI - beta) * rcp(PI - gamma);
//
//                     // Perform a shortest arc rotation.
//                     float3   A = normalize(cross(L, -V));
//                     float3x3 R = RotationFromAxisAngle(A, sin(gamma), cos(gamma));
//
//                     // Rotate the light direction.
//                     L = mul(R, L);
//                 }
//             }
//
//             // TODO: solve in spherical coords?
//             float height = r - R;
//             float  NdotL = dot(N, L);
//             float3 projL = L - N * NdotL;
//             float3 projV = V - N * NdotV;
//             float  phiL  = acos(clamp(dot(projL, projV) * rsqrt(max(dot(projL, projL) * dot(projV, projV), FLT_EPS)), -1, 1));
//
//             TexCoord4D tc = ConvertPositionAndOrientationToTexCoords(height, NdotV, NdotL, phiL);
//
//             float3 radiance = 0; // from 'tEntry' to 'tExit'
//
//             // Single scattering does not contain the phase function.
//             float LdotV = dot(L, V);
//
//             // Air.
//             radiance += lerp(SAMPLE_TEXTURE3D_LOD(_AirSingleScatteringTexture,     s_linear_clamp_sampler, float3(tc.u, tc.v, tc.w0), 0).rgb,
//                              SAMPLE_TEXTURE3D_LOD(_AirSingleScatteringTexture,     s_linear_clamp_sampler, float3(tc.u, tc.v, tc.w1), 0).rgb,
//                              tc.a) * AirPhase(LdotV);
//
//             // Aerosols.
//             // TODO: since aerosols are in a separate texture,
//             // they could use a different max height value for improved precision.
//             radiance += lerp(SAMPLE_TEXTURE3D_LOD(_AerosolSingleScatteringTexture, s_linear_clamp_sampler, float3(tc.u, tc.v, tc.w0), 0).rgb,
//                              SAMPLE_TEXTURE3D_LOD(_AerosolSingleScatteringTexture, s_linear_clamp_sampler, float3(tc.u, tc.v, tc.w1), 0).rgb,
//                              tc.a) * AerosolPhase(LdotV);
//
//             // MS.
//             radiance += lerp(SAMPLE_TEXTURE3D_LOD(_MultipleScatteringTexture,      s_linear_clamp_sampler, float3(tc.u, tc.v, tc.w0), 0).rgb,
//                              SAMPLE_TEXTURE3D_LOD(_MultipleScatteringTexture,      s_linear_clamp_sampler, float3(tc.u, tc.v, tc.w1), 0).rgb,
//                              tc.a) * MS_EXPOSURE_INV;
//
//             if (rayEndsInsideAtmosphere)
//             {
//                 float3 radiance1 = 0; // from 'tFrag' to 'tExit'
//
//                 // TODO: solve in spherical coords?
//                 float height1 = r1 - R;
//                 float  NdotV1 = -cosChi1;
//                 float  NdotL1 = dot(N1, L);
//                 float3 projL1 = L - N1 * NdotL1;
//                 float3 projV1 = V - N1 * NdotV1;
//                 float  phiL1  = acos(clamp(dot(projL1, projV1) * rsqrt(max(dot(projL1, projL1) * dot(projV1, projV1), FLT_EPS)), -1, 1));
//
//                 tc = ConvertPositionAndOrientationToTexCoords(height1, NdotV1, NdotL1, phiL1);
//
//                 // Single scattering does not contain the phase function.
//
//                 // Air.
//                 radiance1 += lerp(SAMPLE_TEXTURE3D_LOD(_AirSingleScatteringTexture,     s_linear_clamp_sampler, float3(tc.u, tc.v, tc.w0), 0).rgb,
//                                   SAMPLE_TEXTURE3D_LOD(_AirSingleScatteringTexture,     s_linear_clamp_sampler, float3(tc.u, tc.v, tc.w1), 0).rgb,
//                                   tc.a) * AirPhase(LdotV);
//
//                 // Aerosols.
//                 // TODO: since aerosols are in a separate texture,
//                 // they could use a different max height value for improved precision.
//                 radiance1 += lerp(SAMPLE_TEXTURE3D_LOD(_AerosolSingleScatteringTexture, s_linear_clamp_sampler, float3(tc.u, tc.v, tc.w0), 0).rgb,
//                                   SAMPLE_TEXTURE3D_LOD(_AerosolSingleScatteringTexture, s_linear_clamp_sampler, float3(tc.u, tc.v, tc.w1), 0).rgb,
//                                   tc.a) * AerosolPhase(LdotV);
//
//                 // MS.
//                 radiance1 += lerp(SAMPLE_TEXTURE3D_LOD(_MultipleScatteringTexture,      s_linear_clamp_sampler, float3(tc.u, tc.v, tc.w0), 0).rgb,
//                                   SAMPLE_TEXTURE3D_LOD(_MultipleScatteringTexture,      s_linear_clamp_sampler, float3(tc.u, tc.v, tc.w1), 0).rgb,
//                                   tc.a) * MS_EXPOSURE_INV;
//
//                 // L(tEntry, tFrag) = L(tEntry, tExit) - T(tEntry, tFrag) * L(tFrag, tExit)
//                 radiance = max(0, radiance - (1 - skyOpacity) * radiance1);
//             }
//
//             radiance *= light.color.rgb; // Globally scale the intensity
//
//             skyColor += radiance;
//         }
//
//         skyColor   = Desaturate(skyColor,   _ColorSaturation);
//         skyOpacity = Desaturate(skyOpacity, _AlphaSaturation) * _AlphaMultiplier;
//
//         float horAngle = acos(cosHor);
//         float chiAngle = acos(cosChi);
//
//         // [start, end] -> [0, 1] : (x - start) / (end - start) = x * rcpLength - (start * rcpLength)
//         // TEMPLATE_3_REAL(Remap01, x, rcpLength, startTimesRcpLength, return saturate(x * rcpLength - startTimesRcpLength))
//         float start    = horAngle;
//         float end      = 0;
//         float rcpLen   = rcp(end - start);
//         float nrmAngle = Remap01(chiAngle, rcpLen, start * rcpLen);
//         // float angle = saturate((0.5 * PI) - acos(cosChi) * rcp(0.5 * PI));
//
//         skyColor *= ExpLerp(_HorizonTint.rgb, _ZenithTint.rgb, nrmAngle, _HorizonZenithShiftPower, _HorizonZenithShiftScale);
//     }
// }

float3 GetViewForwardDir1(float4x4 viewMatrix)
{
    return -viewMatrix[2].xyz;
}

void EvaluateAtmosphericScattering(
    FrameUniforms frameUniform,
    ShaderVariablesVolumetric shaderVariablesVolumetric,
    Texture3D vBufferLighting, SamplerState samplerLinearClamp,
    PositionInputs posInput, float3 V, out float3 color, out float3 opacity)
{
    VBufferUniform vBufferUniform = frameUniform.vBufferUniform;
    VolumetricLightingUniform volumeLightUniform = frameUniform.volumetricLightingUniform;

    float3 _FogColor = volumeLightUniform._FogColor.rgb;
    
    color = opacity = 0;

    // TODO: do not recompute this, but rather pass it directly.
    // Note1: remember the hacked value of 'posInput.positionWS'.
    // Note2: we do not adjust it anymore to account for the distance to the planet. This can lead to wrong results (since the planet does not write depth).
    float fogFragDist = distance(posInput.positionWS, GetCurrentViewPosition(frameUniform));

    if (volumeLightUniform._FogEnabled)
    {
        float4 volFog = float4(0.0, 0.0, 0.0, 0.0);

        float expFogStart = 0.0f;

        if (volumeLightUniform._EnableVolumetricFog != 0)
        {
            bool doBiquadraticReconstruction = volumeLightUniform._VolumetricFilteringEnabled == 0; // Only if filtering is disabled.
            float4 value = SampleVBuffer(TEXTURE3D_ARGS(vBufferLighting, samplerLinearClamp),
                                         posInput.positionNDC,
                                         fogFragDist,
                                         vBufferUniform._VBufferViewportSize,
                                         vBufferUniform._VBufferLightingViewportScale.xyz,
                                         vBufferUniform._VBufferLightingViewportLimit.xyz,
                                         vBufferUniform._VBufferDistanceEncodingParams,
                                         vBufferUniform._VBufferDistanceDecodingParams,
                                         true, doBiquadraticReconstruction, false,
                                         shaderVariablesVolumetric._VBufferRcpSliceCount);

            // TODO: add some slowly animated noise (dither?) to the reconstructed value.
            // TODO: re-enable tone mapping after implementing pre-exposure.
            volFog = DelinearizeRGBA(float4(/*FastTonemapInvert*/(value.rgb), value.a));
            expFogStart = vBufferUniform._VBufferLastSliceDist;
        }

        // TODO: if 'posInput.linearDepth' is computed using 'posInput.positionWS',
        // and the latter resides on the far plane, the computation will be numerically unstable.
        float distDelta = fogFragDist - expFogStart;

        //if (distDelta > 0)
        //{
        //    // Apply the distant (fallback) fog.
        //    float3 positionWS = GetCurrentViewPosition(frameUniform) - V * expFogStart;
        //    float  startHeight = positionWS.y;
        //    float  cosZenith = -V.y;
        
        //    // For both homogeneous and exponential media,
        //    // Integrate[Transmittance[x] * Scattering[x], {x, 0, t}] = Albedo * Opacity[t].
        //    // Note that pulling the incoming radiance (which is affected by the fog) out of the
        //    // integral is wrong, as it means that shadow rays are not volumetrically shadowed.
        //    // This will result in fog looking overly bright.
        
        //    float3 volAlbedo = volumeLightUniform._HeightFogBaseScattering.xyz / volumeLightUniform._HeightFogBaseExtinction;
        //    float  odFallback = OpticalDepthHeightFog(volumeLightUniform._HeightFogBaseExtinction, volumeLightUniform._HeightFogBaseHeight,
        //        volumeLightUniform._HeightFogExponents, cosZenith, startHeight, distDelta);
        //    float  trFallback = TransmittanceFromOpticalDepth(odFallback);
        //    float  trCamera = 1 - volFog.a;
        
        //    volFog.rgb += trCamera * _FogColor * GetCurrentExposureMultiplier(frameUniform) * volAlbedo * (1 - trFallback);
        //    volFog.a = 1 - (trCamera * trFallback);
        //}

        color = volFog.rgb; // Already pre-exposed
        opacity = volFog.a;
    }

}


#endif
