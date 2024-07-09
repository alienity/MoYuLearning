#ifndef HD_SHADOW_CONTEXT_HLSL
#define HD_SHADOW_CONTEXT_HLSL

#include "../../ShaderLibrary/Common.hlsl"
#include "../../Lighting/Shadow/HDShadowManager.hlsl"

// Say to LightloopDefs.hlsl that we have a sahdow context struct define
#define HAVE_HD_SHADOW_CONTEXT

struct HDShadowContext
{
    HDShadowData shadowDatas[64];
    HDDirectionalShadowData directionalShadowData;
#ifdef SHADOWS_SHADOWMASK
    int shadowSplitIndex;
    float fade;
#endif 
};

// HD shadow sampling bindings
#include "../../Lighting/Shadow/HDShadowSampling.hlsl"
#include "../../Lighting/Shadow/HDShadowAlgorithms.hlsl"

// TEXTURE2D(_ShadowmapAtlas);
// TEXTURE2D(_CachedShadowmapAtlas);
// TEXTURE2D(_ShadowmapCascadeAtlas);
// TEXTURE2D(_ShadowmapAreaAtlas);
// TEXTURE2D(_CachedAreaLightShadowmapAtlas);

// StructuredBuffer<HDShadowData>              _HDShadowDatas;
// // Only the first element is used since we only support one directional light
// StructuredBuffer<HDDirectionalShadowData>   _HDDirectionalShadowData;

HDShadowContext InitShadowContext(FrameUniforms frameUniforms)
{
    HDShadowContext         sc;

    sc.shadowDatas = frameUniforms.lightDataUniform.shadowDatas;
    sc.directionalShadowData = frameUniforms.lightDataUniform.directionalShadowData;
#ifdef SHADOWS_SHADOWMASK
    sc.shadowSplitIndex = -1;
    sc.fade = 0.0;
#endif

    return sc;
}

#endif // HD_SHADOW_CONTEXT_HLSL
