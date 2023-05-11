﻿#include "d3d12.hlsli"
#include "Shader.hlsli"
#include "CommonMath.hlsli"
#include "SharedTypes.hlsli"

cbuffer RootConstants : register(b0, space0) { uint meshIndex; };

ConstantBuffer<MeshPerframeBuffer> g_ConstantBufferParams : register(b1, space0);

StructuredBuffer<MeshInstance> g_MeshesInstance : register(t0, space0);

StructuredBuffer<MaterialInstance> g_MaterialsInstance : register(t1, space0);

SamplerState defaultSampler : register(s10);
SamplerComparisonState shadowmapSampler : register(s11);

struct VertexInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float4 tangent  : TANGENT;
    float2 texcoord : TEXCOORD;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 positionWS : TEXCOORD1;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
};

void PrepareMaterial(inout MaterialInputs material)
{

}

float3 SurfaceShading(const MaterialInputs materialInputs, const ShadingData shadingData, const LightData lightData)
{
    return float3(0, 0, 0);
}

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

    MeshInstance mesh = g_MeshesInstance[meshIndex];

    output.positionWS = mul(mesh.localToWorldMatrix, float4(input.position, 1.0f)).xyz;
    output.position   = mul(g_ConstantBufferParams.cameraInstance.projViewMatrix, float4(output.positionWS, 1.0f));

    output.texcoord    = input.texcoord;

    float3x3 normalMat = transpose((float3x3)mesh.localToWorldMatrixInverse);

    output.normal      = normalize(mul(normalMat, input.normal));
    output.tangent.xyz = normalize(mul(normalMat, input.tangent.xyz));
    output.tangent.w   = input.tangent.w;

    return output;
}

float4 PSMain(VertexOutput input) : SV_Target0
{
    MeshInstance     mesh     = g_MeshesInstance[meshIndex];
    MaterialInstance material = g_MaterialsInstance[mesh.materialIndex];

    StructuredBuffer<PerMaterialUniformBuffer> matBuffer = ResourceDescriptorHeap[material.uniformBufferIndex];

    float4 baseColorFactor = matBuffer[0].baseColorFactor;
    float  normalStength    = matBuffer[0].normalScale;

    Texture2D<float4> baseColorTex = ResourceDescriptorHeap[material.baseColorIndex];
    Texture2D<float3> normalTex    = ResourceDescriptorHeap[material.normalIndex];

    float4 baseColor = baseColorTex.Sample(defaultSampler, input.texcoord);
    float3 vNt       = normalTex.Sample(defaultSampler, input.texcoord) * 2.0f - 1.0f;

    baseColor = baseColor * baseColorFactor;

    // http://www.mikktspace.com/
    float3 vNormal    = input.normal.xyz;
    float3 vTangent   = input.tangent.xyz;
    float3 vBiTangent = input.tangent.w * cross(vNormal, vTangent);
    float3 vNout      = normalize(vNt.x * vTangent + vNt.y * vBiTangent + vNt.z * vNormal);

    vNout = vNout * normalStength;

    float3 positionWS = input.positionWS;
    float3 viewPos    = g_ConstantBufferParams.cameraInstance.cameraPosition;

    float3 ambientColor = g_ConstantBufferParams.ambient_light * 0.1f;

    float3 outColor = ambientColor;

    // direction light
    {
        float3 lightForward   = g_ConstantBufferParams.scene_directional_light.direction;
        float3 lightColor     = g_ConstantBufferParams.scene_directional_light.color;
        float  lightStrength  = g_ConstantBufferParams.scene_directional_light.intensity;
        uint   shadowmap      = g_ConstantBufferParams.scene_directional_light.shadowmap;

        lightColor = lightColor * lightStrength;

        float3 lightDir = -lightForward;

        float3 viewDir    = normalize(viewPos - positionWS);
        float3 halfwayDir = normalize(lightDir + viewDir);

        float  diff    = max(dot(vNout, lightDir), 0.0f);
        float3 diffuse = lightColor * diff;

        float  spec     = pow(max(dot(vNout, halfwayDir), 0.0f), 32);
        float3 specular = lightColor * spec;

        float3 directionLightColor = diffuse + specular;

        if (shadowmap == 1)
        {
            float4x4 lightViewProjMat = g_ConstantBufferParams.scene_directional_light.light_proj_view;

            float4 fragPosLightSpace = mul(lightViewProjMat, float4(positionWS, 1.0f));

            // perform perspective divide
            float3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
            // transform to [0,1] range
            projCoords.xy = projCoords.xy * 0.5 + 0.5;

            // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
            Texture2D<float> shadowMap = ResourceDescriptorHeap[g_ConstantBufferParams.scene_directional_light.shadowmap_srv_index];

            //float shadow_bias = 0.0004f;
            float shadow_bias = max(0.001f * (1.0 - dot(vNormal, lightDir)), 0.0003f);  

            float shadowTexelSize = 1.0f / g_ConstantBufferParams.scene_directional_light.shadowmap_width;

            // PCF
            float pcfDepth = projCoords.z;

            float fShadow = 0.0f;
            for (int x = -1; x <= 1; x++)
            {
                for (int y = -1; y <= 1; y++)
                {
                    float2 tempShadowTexCoord = float2(projCoords.x, 1.0f - projCoords.y) + float2(x, y) * shadowTexelSize;
                    float tempfShadow = shadowMap.SampleCmpLevelZero(shadowmapSampler, tempShadowTexCoord, pcfDepth + shadow_bias);
                    fShadow += tempfShadow;
                }
            }
            fShadow /= 9.0f;

            directionLightColor *= fShadow;
        }
        outColor = outColor + directionLightColor;
    }

    uint point_light_num = g_ConstantBufferParams.point_light_num;
    uint i = 0;
    for (i = 0; i < point_light_num; i++)
    {
        float3 lightPos      = g_ConstantBufferParams.scene_point_lights[i].position;
        float3 lightColor    = g_ConstantBufferParams.scene_point_lights[i].color;
        float  lightStrength = g_ConstantBufferParams.scene_point_lights[i].intensity;
        float  lightRadius   = g_ConstantBufferParams.scene_point_lights[i].radius;

        lightColor = lightColor * lightStrength;

        float lightDistance = length(lightPos - positionWS);
        float attenuation   = saturate(1.0f - lightDistance / lightRadius);
        attenuation         = attenuation * attenuation;

        float3 lightDir = normalize(lightPos - positionWS);
        float3 viewDir  = normalize(viewPos - positionWS);
        float3 halfwayDir = normalize(lightDir + viewDir);

        float  diff    = max(dot(vNout, lightDir), 0.0f);
        float3 diffuse = lightColor * diff * attenuation;

        float  spec     = pow(max(dot(vNout, halfwayDir), 0.0f), 32);
        float3 specular = lightColor * spec * attenuation;

        outColor = outColor + diffuse + specular;
    }

    uint spot_light_num = g_ConstantBufferParams.spot_light_num;
    for (i = 0; i < spot_light_num; i++)
    {
        float3 lightPos          = g_ConstantBufferParams.scene_spot_lights[i].position;
        float3 lightForward      = g_ConstantBufferParams.scene_spot_lights[i].direction;
        float3 lightColor        = g_ConstantBufferParams.scene_spot_lights[i].color;
        float  lightStrength     = g_ConstantBufferParams.scene_spot_lights[i].intensity;
        float  lightRadius       = g_ConstantBufferParams.scene_spot_lights[i].radius;
        float  lightInnerRadians = g_ConstantBufferParams.scene_spot_lights[i].inner_radians;
        float  lightOuterRadians = g_ConstantBufferParams.scene_spot_lights[i].outer_radians;
        uint   shadowmap         = g_ConstantBufferParams.scene_spot_lights[i].shadowmap;

        float lightInnerCutoff = cos(lightInnerRadians * 0.5f);
        float lightOuterCutoff = cos(lightOuterRadians * 0.5f);

        lightColor = lightColor * lightStrength;

        float3 lightDir = normalize(lightPos - positionWS);

        float theta = dot(lightDir, normalize(-lightForward));

        if (theta < lightOuterCutoff)
            continue;

        float lightDistance = length(lightPos - positionWS);
        float attenuation   = saturate(1.0f - lightDistance / lightRadius);
        attenuation         = attenuation * attenuation;

        float epsilon = lightInnerCutoff - lightOuterCutoff;
        float intensity = saturate((theta - lightOuterCutoff) / epsilon); 

        float3 viewDir    = normalize(viewPos - positionWS);
        float3 halfwayDir = normalize(lightDir + viewDir);

        float  diff    = max(dot(vNout, lightDir), 0.0f);
        float3 diffuse = lightColor * diff * attenuation * intensity;

        float  spec     = pow(max(dot(vNout, halfwayDir), 0.0f), 32);
        float3 specular = lightColor * spec * attenuation * intensity;

        float3 spotLightColor = diffuse + specular;

        if (shadowmap == 1)
        {
            float4x4 lightViewProjMat = g_ConstantBufferParams.scene_spot_lights[i].light_proj_view;

            float4 fragPosLightSpace = mul(lightViewProjMat, float4(positionWS, 1.0f));

            // perform perspective divide
            float3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
            // transform to [0,1] range
            projCoords.xy = projCoords.xy * 0.5 + 0.5;

            // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
            Texture2D<float> shadowMap = ResourceDescriptorHeap[g_ConstantBufferParams.scene_spot_lights[i].shadowmap_srv_index];

            // float shadow_bias = 0.0004f;
            float shadow_bias = max(0.001f * (1.0 - dot(vNormal, lightDir)), 0.0003f);

            float shadowTexelSize = 1.0f / g_ConstantBufferParams.scene_spot_lights[i].shadowmap_width;

            // PCF
            float pcfDepth = projCoords.z;

            float fShadow = 0.0f;
            for (int x = -1; x <= 1; x++)
            {
                for (int y = -1; y <= 1; y++)
                {
                    float2 tempShadowTexCoord = float2(projCoords.x, 1.0f - projCoords.y) + float2(x, y) * shadowTexelSize;
                    float  tempfShadow = shadowMap.SampleCmpLevelZero(shadowmapSampler, tempShadowTexCoord, pcfDepth + shadow_bias);
                    fShadow += tempfShadow;
                }
            }
            fShadow /= 9.0f;

            spotLightColor *= fShadow;
        }

        outColor = outColor + spotLightColor;
    }

    outColor = outColor * baseColor.rgb;

    return float4(outColor.rgb, baseColor.a);
}


/*
// https://devblogs.microsoft.com/directx/hlsl-shader-model-6-6/
// https://microsoft.github.io/DirectX-Specs/d3d/HLSL_ShaderModel6_6.html
// https://devblogs.microsoft.com/directx/in-the-works-hlsl-shader-model-6-6/
float4 PSMain(VertexOutput input) : SV_Target0
{
    MeshInstance     mesh     = g_MeshesInstance[meshIndex];
    MaterialInstance material = g_MaterialsInstance[mesh.materialIndex];

    //// https://github.com/microsoft/DirectXShaderCompiler/issues/2193
    //ByteAddressBuffer matFactorsBuffer = ResourceDescriptorHeap[material.uniformBufferIndex];
    //float4 baseColorFactor = matFactorsBuffer.Load<float4>(0);

    // https://docs.microsoft.com/en-us/windows/win32/direct3d11/direct3d-11-advanced-stages-cs-resources
    StructuredBuffer<PerMaterialUniformBuffer> matFactors = ResourceDescriptorHeap[material.uniformBufferIndex];
    float4 baseColorFactor = matFactors[0].baseColorFactor;

    Texture2D<float4> baseColorTex = ResourceDescriptorHeap[material.baseColorIndex];
    Texture2D<float3> normalTex = ResourceDescriptorHeap[material.normalIndex];

    float4 baseColor = baseColorTex.Sample(defaultSampler, input.texcoord);

    float3 normalVal = normalTex.Sample(defaultSampler, input.texcoord) * 2.0f - 1.0f;

    baseColor = baseColor * baseColorFactor;

    return float4(normalVal.rgb, 1);
}
*/