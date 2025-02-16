
#define OPAQUE_FOG_PASS

// Defined for caustics
#define SHADOW_LOW
#define AREA_SHADOW_LOW

#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/Color.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"
#include "../../Lighting/Lighting.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"
#include "../../Sky/SkyUtils.hlsl"

Texture2D<float4>(_ColorTextureMS);
Texture2D<float>(_DepthTextureMS);
Texture2D _ColorTexture;
float _MultipleScatteringIntensity;

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

    #ifdef DEBUG_DISPLAY
    if (_DebugFullScreenMode == FULLSCREENDEBUGMODE_VOLUMETRIC_FOG)
        output.color = float4(debugColor, 0.0f);
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
    float4x4 _PixelCoordToViewDirWS;
    Texture2D<float> cameraDepthTex;
    
    float2 positionSS = input.positionCS.xy;
    float3 V          = GetSkyViewDirWS(positionSS, _PixelCoordToViewDirWS);
    float  depth      = LoadCameraDepth(cameraDepthTex, positionSS);

    PositionInputs posInput = GetPositionInput(input.positionCS.xy, _ScreenSize.zw, depth, UNITY_MATRIX_I_VP, UNITY_MATRIX_V);;
    // PositionInputs posInput = GetPositionInput(input, depth);

    float3 volColor, volOpacity;
    EvaluateAtmosphericScattering(posInput, V, volColor, volOpacity);

    return OutputFog(volColor, volOpacity);
}

