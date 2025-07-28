

//#include "./TerrainCommonInput.hlsl"
#include "../../ShaderLibrary/Common.hlsl"

//struct PatchBoundsArgsStruct
//{
//    D3D12_DRAW_ARGUMENTS drawArgs;
//};

//cbuffer RootConstants : register(b0, space0)
//{
//    uint PatchBoundsListCounterBufferIndex;
//    uint camPatchBoundsCmdSigBufferIndex;
//};

ByteAddressBuffer PatchCounterBuffer : register(t0, space0);
RWStructuredBuffer<D3D12_DRAW_ARGUMENTS> camPatchArgStruct : register(u0, space0);

[numthreads(1, 1, 1)]
void GenerateTerrainBoundsCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    //ByteAddressBuffer PatchCounterBuffer = ResourceDescriptorHeap[PatchBoundsListCounterBufferIndex];
    //RWStructuredBuffer<PatchBoundsArgsStruct> camPatchArgStruct = ResourceDescriptorHeap[camPatchBoundsCmdSigBufferIndex];

    int BoundsCount = PatchCounterBuffer.Load(0);
    
    D3D12_DRAW_ARGUMENTS drawArgs;
    drawArgs.VertexCountPerInstance = 36;
    drawArgs.InstanceCount = BoundsCount;
    drawArgs.StartVertexLocation = 0;
    drawArgs.StartInstanceLocation = 0;

    //PatchBoundsArgsStruct drawArgsStruct;
    //drawArgsStruct.drawArgs = drawArgs;
    
    camPatchArgStruct[0] = drawArgs;
}