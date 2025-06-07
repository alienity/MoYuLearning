
#define OPAQUE_FOG_PASS

// Defined for caustics
#define SHADOW_LOW
#define AREA_SHADOW_LOW

#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/Color.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"
#include "../../Lighting/Lighting.hlsl"
#include "../../Tools/VolumeLighting/VolumetricLightingCommon.hlsl"
#include "../../Sky/SkyUtils.hlsl"

ConstantBuffer<FrameUniforms> _FrameUniforms : register(b0, space0);
ConstantBuffer<ShaderVariablesVolumetric> _ShaderVariablesVolumetric : register(b1, space0);
Texture3D _VBufferLighting : register(t0, space0);
Texture2D<float> _DepthTextureMS : register(t1, space0);

SamplerState sampler_LinearClamp : register(s10);
SamplerState sampler_LinearRepeat : register(s11);
SamplerState sampler_PointClamp : register(s12);
SamplerState sampler_PointRepeat : register(s13);

struct Attributes
{
    uint vertexID : SV_VertexID;
};

struct Varyings
{
    float4 positionCS : SV_POSITION;
};

struct FragOutput
{
    float4 color : SV_Target0;
    #if defined(OUTPUT_TRANSMITTANCE_BUFFER)
    float2 fogTransmittance : SV_Target1;
    #endif
};

Varyings Vert(Attributes input)
{
    Varyings output;
    output.positionCS = GetFullScreenTriangleVertexPosition(input.vertexID);
    return output;
}

FragOutput ComputeFragmentOutput(float4 color, float3 fogOpacity, float3 debugColor)
{
    FragOutput output;

    output.color = color;

    #if defined(OUTPUT_TRANSMITTANCE_BUFFER)
    float finalOpacity = (fogOpacity.x + fogOpacity.y + fogOpacity.z) / 3.0f;
    output.fogTransmittance = 1 - finalOpacity;
    #endif

    return output;
}

// Helpers to reduce duplication
FragOutput OutputFog(float3 volColor, float3 volOpacity)
{
    return ComputeFragmentOutput(float4(volColor, 1.0 - volOpacity.x), volOpacity, volColor);
}

FragOutput OutputFog(float4 surfColor, float3 volColor, float3 volOpacity, float3 fogOpacity)
{
    // Premultiplied alpha (over operator), preserve alpha for the alpha channel for compositing
    return ComputeFragmentOutput(float4(volColor + (1 - volOpacity) * surfColor.rgb, surfColor.a), fogOpacity, volColor);
}

#define ATMOSPHERE_NO_AERIAL_PERSPECTIVE
#include "../../Lighting/AtmosphericScattering/AtmosphericScattering.hlsl"

FragOutput Frag(Varyings input)
{
    CameraUniform cameraUniform = _FrameUniforms.cameraUniform;
    float4 _ScreenSize = _FrameUniforms.baseUniform._ScreenSize;
    
    float4x4 _PixelCoordToViewDirWS = _ShaderVariablesVolumetric._VBufferCoordToViewDirWS;
    
    float2 positionSS = input.positionCS.xy;
    // float3 V = normalize(mul(UNITY_MATRIX_I_VP(cameraUniform), float4((positionSS * 2 - 1), 0, 0)).xyz);
    float3 V          = GetSkyViewDirWS(positionSS, _PixelCoordToViewDirWS);
    float  depth      = LoadCameraDepth(_DepthTextureMS, positionSS);

    PositionInputs posInput = GetPositionInput(input.positionCS.xy, _ScreenSize.zw, depth,
        UNITY_MATRIX_I_VP(cameraUniform), UNITY_MATRIX_V(cameraUniform));;
    // PositionInputs posInput = GetPositionInput(input, depth);

    float3 volColor, volOpacity;
    EvaluateAtmosphericScattering(
        _FrameUniforms, _ShaderVariablesVolumetric,
        _VBufferLighting, sampler_LinearClamp, posInput, V, volColor, volOpacity);

    return OutputFog(volColor, 1 - volOpacity);
}

