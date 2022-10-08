﻿#include "d3d12.hlsli"
#include "Shader.hlsli"
#include "Math.hlsli"
#include "SharedTypes.hlsli"

struct CommandSignatureParams
{
	uint						 MeshIndex;
	D3D12_VERTEX_BUFFER_VIEW	 VertexBuffer;
	D3D12_INDEX_BUFFER_VIEW		 IndexBuffer;
	D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments;
};

ConstantBuffer<MeshPerframeStorageBufferObject> g_ConstantBufferParams : register(b0, space0);
StructuredBuffer<MeshInstance>                  g_MeshesInstance : register(t0, space0);

AppendStructuredBuffer<CommandSignatureParams> g_DrawShadowmapCommandBuffer : register(u0, space0);

[numthreads(128, 1, 1)]
void CSMain(CSParams Params)
{
	// Each thread processes one mesh instance
	// Compute index and ensure is within bounds
	uint index = (Params.GroupID.x * 128) + Params.GroupIndex;
    if (index < g_ConstantBufferParams.total_mesh_num)
	{
        MeshInstance mesh = g_MeshesInstance[index];

		BoundingBox aabb;
        mesh.boundingBox.Transform(mesh.localToWorldMatrix, aabb);

		Frustum frustum = ExtractPlanesDX(g_ConstantBufferParams.scene_directional_light.directional_light_proj_view);

		bool visible = FrustumContainsBoundingBox(frustum, aabb) != CONTAINMENT_DISJOINT;
		if (visible)
		{
			CommandSignatureParams command;
			command.MeshIndex			 = index;
            command.VertexBuffer         = mesh.vertexBuffer;
            command.IndexBuffer          = mesh.indexBuffer;
            command.DrawIndexedArguments = mesh.drawIndexedArguments;
            g_DrawShadowmapCommandBuffer.Append(command);
		}

	}
}