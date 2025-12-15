#ifndef TERRAIN_COMMON_INPUT
#define TERRAIN_COMMON_INPUT

#ifndef TERRAIN_BOUNDS_DEBUG
#define TERRAIN_BOUNDS_DEBUG 1
#endif

#ifdef _CPP_MACRO_
#define uint glm::uint
#define uint2 glm::uvec2
#define uint3 glm::uvec3
#define uint4 glm::uvec4
#define int2 glm::int2
#define int3 glm::int3
#define int4 glm::int4
#define float2 glm::fvec2
#define float3 glm::fvec3
#define float4 glm::fvec4
#define float4x4 glm::float4x4
#define float2x4 glm::float2x4
#else
#include "../../ShaderLibrary/Common.hlsl"
#endif

//The maximum LOD level is 5
#define MAX_TERRAIN_LOD 5
#define MAX_NODE_ID 34124

// At Max LOD, the world consists of 5x5 Nodes
#define MAX_LOD_NODE_COUNT 5

// A PatchMesh consists of 16x16 grids
#define PATCH_MESH_GRID_COUNT 16

// A PatchMesh has a side length of 8 meters
#define PATCH_MESH_SIZE 8

// A Node is divided into 8x8 Patches
#define PATCH_COUNT_PER_NODE 8

// A PatchMesh grid cell size is 0.5x0.5
#define PATCH_MESH_GRID_SIZE 0.5
#define SECTOR_COUNT_WORLD 160

struct TerrainRenderPatch
{
    float2 position;
    float minHeight;
    float maxHeight;
    uint4 lodTrans;
    uint lod;
};

struct TerrainPatchBounds
{
    float3 minPosition;
    float3 maxPosition;
};

#if TERRAIN_BOUNDS_DEBUG
struct BoundsDebug
{
    TerrainPatchBounds bounds;
    float4 color;
};
#endif

struct TerrainRenderData
{
    float4x4 objectToWorldMatrix;
    float4x4 worldToObjectMatrix;
    float4x4 prevObjectToWorldMatrix;
    float4x4 prevWorldToObjectMatrix;
    float4 terrainSize;
};

/*
* For WorldLodParams
- nodeSize is the Node side length (meters)
- patchExtent equals nodeSize/16
- nodeCount equals WorldSize/nodeSize
- sectorCountPerNode equals 2^lod
 */
struct TerrainConsData
{
    float4x4 TerrainModelMatrix;
    float4x4 CameraViewProj;
    float3 CameraPositionWS; // Camera world space coordinates
    int BoundsHeightRedundance; // Bounding box reserves redundant space in height direction to cope with insufficient precision of MinMaxHeightTexture
    float3 WorldSize; // World size
    float _HizDepthBias;
    float4 HizDepthMapSize;
    float4 NodeEvaluationC; // Node evaluation coefficient. x is the distance coefficient
    float4 WorldLodParams[MAX_TERRAIN_LOD+1]; // (nodeSize,patchExtent,nodeCount,sectorCountPerNode)
    uint NodeIDOffsetOfLOD[MAX_TERRAIN_LOD+1];
};

#ifdef _CPP_MACRO_
#undef uint
#undef uint2
#undef uint3
#undef uint4
#undef int2
#undef int3
#undef int4
#undef float2
#undef float3
#undef float4
#undef float4x4
#undef float2x4
#endif

#endif
