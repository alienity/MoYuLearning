#pragma once
#ifndef GEOMETRY_MESH_TOOL_H
#define GEOMETRY_MESH_TOOL_H
#include "runtime/function/render/render_common.h"
#include "runtime/core/math/moyu_math2.h"

#include <vector>

namespace MoYu
{
    namespace Geometry
    {
        typedef D3D12MeshVertexStandard Vertex;

        Vertex VertexInterpolate(Vertex& v0, Vertex& v1, float t);
        Vertex VertexVectorBuild(Vertex& init, Vertex& temp);
        Vertex VertexTransform(Vertex& v0, glm::mat4x4& matrix);

        struct BasicMesh
        {
            std::vector<Vertex> vertices;
            std::vector<int>    indices;
        };

        AABB ToAxisAlignedBox(const BasicMesh& basicMesh);
        StaticMeshData ToStaticMesh(const BasicMesh& basicMesh);

    }

    namespace Geometry
    {

        struct TriangleMesh : BasicMesh
        {
            TriangleMesh(float width = 1.0f);

            static BasicMesh ToBasicMesh(float width = 1.0f);
        };

        struct SquareMesh : BasicMesh
        {
            SquareMesh(float width = 1.0f);

            static BasicMesh ToBasicMesh(float width = 1.0f);
        };

        struct TerrainPlane : BasicMesh
        {
            TerrainPlane(int gridCount = 16, float meshSize = 8);

            static TerrainPlane ToBasicMesh(int meshGridCount = 16, float meshSize = 8);
        };


    } // namespace Geometry

    /*
    namespace TerrainGeometry
    {
        // https://zhuanlan.zhihu.com/p/352850047
        // �ؿ��mesh�ǿ��Ϊ4��mesh��һ����5������
        struct TerrainPatchMesh
        {
            TerrainPatchMesh(uint32_t totallevel, float scale, uint32_t treeIndex, glm::u64vec2 bitOffset, glm::float2 localOffset);

            uint32_t     totalLevel; // tree���ܹ�����
            float        localScale; // ��ǰpatch������
            float        localMaxHeight; // ��ǰpatch�����߶�
            uint32_t     linearTreeIndex; // ��չ����tree�е�λ��
            glm::u64vec2 localTreePosBit; // ��ʶ��tree�еľ���λ�ã���λ��ʶλ��
            glm::float2  localOffset; // ��ǰpatch�ڴ��ͼ�е���������ƫ��

            std::vector<D3D12TerrainPatch> vertices;
            std::vector<int> indices;
        };

        AABB ToAxisAlignedBox(const TerrainPatchMesh& basicMesh);
        StaticMeshData ToStaticMesh(const TerrainPatchMesh& basicMesh);
    }
    */
}
#endif