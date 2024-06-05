//-------------------------------------------------------------------------------------
// FragInputs
// This structure gather all possible varying/interpolator for this shader.
//-------------------------------------------------------------------------------------

#ifndef FRAG_INPUTS_ENABLE_STRIPPING
    #define FRAG_INPUTS_USE_TEXCOORD0
    #define FRAG_INPUTS_USE_TEXCOORD1
    #define FRAG_INPUTS_USE_TEXCOORD2
    #define FRAG_INPUTS_USE_TEXCOORD3
#endif

struct FragInputs
{
    // Contain value return by SV_POSITION (That is name positionCS in PackedVarying).
    // xy: unormalized screen position (offset by 0.5), z: device depth, w: depth in view space
    // Note: SV_POSITION is the result of the clip space position provide to the vertex shaders that is transform by the viewport
    float4 positionSS; // In case depth offset is use, positionRWS.w is equal to depth offset
    float3 positionRWS; // Relative camera space position
    float3 positionPredisplacementRWS; // Relative camera space position
    float2 positionPixel; // Pixel position (VPOS)

    float4 texCoord0;
    float4 texCoord1;
    float4 texCoord2;
    float4 texCoord3;

    float4 color; // vertex color

    // TODO: confirm with Morten following statement
    // Our TBN is orthogonal but is maybe not orthonormal in order to be compliant with external bakers (Like xnormal that use mikktspace).
    // (xnormal for example take into account the interpolation when baking the normal and normalizing the tangent basis could cause distortion).
    // When using tangentToWorld with surface gradient, it doesn't normalize the tangent/bitangent vector (We instead use exact same scale as applied to interpolated vertex normal to avoid breaking compliance).
    // this mean that any usage of tangentToWorld[1] or tangentToWorld[2] outside of the context of normal map (like for POM) must normalize the TBN (TCHECK if this make any difference ?)
    // When not using surface gradient, each vector of tangentToWorld are normalize (TODO: Maybe they should not even in case of no surface gradient ? Ask Morten)
    float3x3 tangentToWorld;

    uint primitiveID; // Only with fullscreen pass debug currently - not supported on all platforms

    // For two sided lighting
    bool isFrontFace;
};

void AdjustFragInputsToOffScreenRendering(inout FragInputs input, bool offScreenRenderingEnabled, float offScreenRenderingFactor)
{
    // We need to readapt the SS position as our screen space positions are for a low res buffer, but we try to access a full res buffer.
    input.positionSS.xy = offScreenRenderingEnabled ? (uint2) round(input.positionSS.xy * offScreenRenderingFactor) : input.positionSS.xy;
    input.positionPixel = offScreenRenderingEnabled ? (uint2) round(input.positionPixel * offScreenRenderingFactor) : input.positionPixel;
}
