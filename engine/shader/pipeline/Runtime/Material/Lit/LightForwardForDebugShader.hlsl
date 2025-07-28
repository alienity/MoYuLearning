
#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/Color.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"

#include "../../Tools/Terrain/TerrainCommonInput.hlsl"
#include "../../Core/Utilities/GeometryUtils.c.hlsl"


ConstantBuffer<FrameUniforms> _FrameUniforms : register(b0, space0);
ConstantBuffer<TerrainConsData> _InConsBuffer : register(b1, space0);
StructuredBuffer<TerrainRenderPatch> terrainRenderPatchBuffer : register(t0, space0);
StructuredBuffer<TerrainRenderData> terrainRenderDataBuffer : register(t1, space0);

SamplerState sampler_LinearClamp : register(s10);
SamplerState sampler_LinearRepeat : register(s11);
SamplerState sampler_PointClamp : register(s12);
SamplerState sampler_PointRepeat : register(s13);

float GetPatchExtent(TerrainConsData inConsBuffer, uint lod)
{
    return inConsBuffer.WorldLodParams[lod].y;
}

TerrainPatchBounds GetPatchBounds(TerrainConsData inConsBuffer, TerrainRenderPatch patch)
{
    float halfSize = GetPatchExtent(inConsBuffer, patch.lod);
    TerrainPatchBounds bounds;
    float3 boundsMin, boundsMax;
    boundsMin.xz = patch.position - halfSize;
    boundsMax.xz = patch.position + halfSize;
    boundsMin.y = patch.minHeight;
    boundsMax.y = patch.maxHeight;

    bounds.minPosition = boundsMin;
    bounds.maxPosition = boundsMax;
    return bounds;
}


// https://iquilezles.org/www/articles/distfunctions/distfunctions.htm
float DistanceToAABB(float3 p, float3 b)
{
    float3 q = abs(p) - b;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

// Optimized version of https://www.sciencedirect.com/topics/computer-science/oriented-bounding-box
float DistanceDistanceToOBB(float3 p, OrientedBBox obb)
{
    float3 offset = p - obb.center;
    float3 boxForward = normalize(cross(obb.right, obb.up));
    float3 axisAlignedPoint = float3(dot(offset, normalize(obb.right)), dot(offset, normalize(obb.up)), dot(offset, boxForward));

    return DistanceToAABB(axisAlignedPoint, float3(obb.extentX, obb.extentY, obb.extentZ));
}

float3 ComputeCubeVertexPositionRWS(OrientedBBox obb, float3 minPositionRWS, float3 vertexMask)
{
    float3x3 obbFrame   = float3x3(obb.right, obb.up, cross(obb.right, obb.up));
    float3   obbExtents = float3(obb.extentX, obb.extentY, obb.extentZ);
    return mul((vertexMask * 2 - 1) * obbExtents, (obbFrame)) + obb.center;
}

// vertexMask contains the vertices of the cube in this order
//    6 .+------+ 7
//    .' |    .'|
// 2 +---+--+'3 |
//   |   |  |   |
//   | 4.+--+---+ 5
//   |.'    | .'
// 0 +------+' 1

static const float3 cubeVertices[8] = {
    float3(0, 0, 0), // 0
    float3(1, 0, 0), // 1
    float3(0, 1, 0), // 2
    float3(1, 1, 0), // 3
    float3(0, 0, 1), // 4
    float3(1, 0, 1), // 5
    float3(0, 1, 1), // 6
    float3(1, 1, 1)  // 7
};

static const uint cubeIndices[36] = {
    // 前面 (Z=0)
    0, 1, 2, 2, 1, 3,
    // 后面 (Z=1)
    5, 4, 7, 7, 4, 6,
    // 左面 (X=0)
    4, 0, 6, 6, 0, 2,
    // 右面 (X=1)
    1, 5, 3, 3, 5, 7,
    // 底面 (Y=0)
    4, 5, 0, 0, 5, 1,
    // 顶面 (Y=1)
    2, 3, 6, 6, 3, 7
};

float3 GetCubeMeshVertexPosition(uint vertexID)
{
    uint index = cubeIndices[vertexID % 36];
    return cubeVertices[index];
}

float3 GetCubeMeshVertexPosition(uint vertexID, float3 center, float3 size)
{
    uint index = cubeIndices[vertexID % 36];
    float3 localPosition = cubeVertices[index];
    return (localPosition - 0.5) * size + center;
}

struct Attributes
{
    uint vertexID : SV_VertexID;
    uint instanceID : SV_InstanceID;
};

struct Varyings
{
    float4 positionCS : SV_POSITION;
};

struct FragOutput
{
    float4 color : SV_Target0;
};

Varyings Vert(Attributes input)
{
    TerrainRenderPatch terrainRenderPatch = terrainRenderPatchBuffer[input.instanceID];
    TerrainRenderData terrainRenderData = terrainRenderDataBuffer[0];

    TerrainPatchBounds patchBounds = GetPatchBounds(_InConsBuffer, terrainRenderPatch);
    float3 patchCenter = (patchBounds.minPosition + patchBounds.maxPosition) * 0.5f;
    float3 patchExtent = (patchBounds.maxPosition - patchBounds.minPosition) * 0.5f;
    float3 vertexPositionWS = GetCubeMeshVertexPosition(input.vertexID, patchCenter, patchExtent);
    float4 vertexPositionCS = mul(UNITY_MATRIX_VP(_FrameUniforms.cameraUniform), float4(vertexPositionWS, 1));
    
    Varyings output;
    output.positionCS = vertexPositionCS;
    return output;
}

FragOutput Frag(Varyings input)
{
    float4 volColor = float4(0.5, 0.5, 0.5, 1);
    FragOutput output;
    output.color = volColor;
    return output;
}

