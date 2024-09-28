#pragma once
#include "runtime/function/render/render_common.h"
#include "runtime/resource/basic_geometry/mesh_tools.h"
#include <vector>

namespace MoYu
{
    class RenderResource;

    class TerrainRenderHelper
    {
    public:
        TerrainRenderHelper();
        ~TerrainRenderHelper();

        void InitTerrainRenderer(
            SceneTerrainRenderer& sceneTerrainRenderer, 
            SceneTerrainRenderer& cachedSceneTerrainRenderer, 
            InternalTerrainRenderer& internalTerrainRenderer,
            RenderResource* renderResource);

        bool updateInternalTerrainRenderer(
            RenderResource* renderResource,
            SceneTerrainRenderer& scene_terrain_renderer,
            SceneTerrainRenderer& cached_terrain_renderer,
            InternalTerrainRenderer& internal_terrain_renderer,
            glm::float4x4            model_matrix,
            glm::float4x4            model_matrix_inv,
            glm::float4x4            prev_model_matrix,
            glm::float4x4            prev_model_matrix_inv,
            bool                     has_initialized = false);

        float       GetTerrainHeight(InternalTerrain* internalTerrain, glm::float2 localXZ);
        glm::float3 GetTerrainNormal(InternalTerrain* internalTerrain, glm::float2 localXZ);

    private:
        void InitTerrainBasicInfo(
            SceneTerrainRenderer& sceneTerrainRenderer, 
            InternalTerrain& internalTerrain);

        void InitTerrainPatchMesh(
            SceneTerrainRenderer& sceneTerrainRenderer, 
            InternalTerrain& internalTerrain, 
            RenderResource* renderResource);

        void InitTerrainHeightAndNormalMap(
            SceneTerrainRenderer& sceneTerrainRenderer, 
            SceneTerrainRenderer& cachedSceneTerrainRenderer, 
            InternalTerrain& internalTerrain, 
            RenderResource* renderResource);

        void InitTerrainBaseTextures(
            SceneTerrainRenderer& sceneTerrainRenderer, 
            SceneTerrainRenderer& cachedSceneTerrainRenderer, 
            InternalMaterial& internalTerrainMaterial,
            RenderResource* renderResource);

    private:
        glm::float2 last_cam_xz;

        bool mIsHeightmapInit;
        bool mIsNaterialInit;
        bool mIsPatchMeshInit;

        MoYu::Geometry::TerrainPlane terrainPatch;

        InternalStandardLightMaterial mInternalBaseTextures;

        std::shared_ptr<MoYu::MoYuScratchImage> terrain_heightmap_scratch;
        std::shared_ptr<MoYu::MoYuScratchImage> terrain_normalmap_scratch;

        std::shared_ptr<RHI::D3D12Texture> terrain_heightmap;
        std::shared_ptr<RHI::D3D12Texture> terrain_normalmap;
    };

    /*
     * ����WorldLodParams
     * - nodeSizeΪNode�ı߳�(��)
     * - patchExtent����nodeSize/16
     * - nodeCount����WorldSize/nodeSize
     * - sectorCountPerNode����2^lod
     */
    struct TerrainConsBuffer
    {
        glm::float3 TerrainOffsetWS; // Terrain������ռ��ƫ��
        float Padding0;
        glm::float4x4 CameraViewProj;
        glm::float3 CameraPositionWS; // �������ռ�����
        int BoundsHeightRedundance; //��Χ���ڸ߶ȷ�����������ռ䣬Ӧ��MinMaxHeightTexture�ľ��Ȳ���
        glm::float3 WorldSize; //�����С
        float Padding1;
        glm::float4 NodeEvaluationC; //�ڵ�����ϵ����xΪ����ϵ��
        glm::float4 WorldLodParams[6]; // (nodeSize,patchExtent,nodeCount,sectorCountPerNode)
        glm::uint NodeIDOffsetOfLOD[6];
    };

} // namespace MoYu
