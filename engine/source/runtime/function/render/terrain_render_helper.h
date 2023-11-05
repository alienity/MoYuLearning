#pragma once
#include "runtime/function/render/render_common.h"
#include <vector>

namespace MoYu
{
    struct TRect
    {
        float xMin;
        float yMin;
        float width;
        float height;

        bool Contains(glm::float2 center)
        {
            bool isInCenter = center.x > xMin;
            isInCenter &= center.y > yMin;
            isInCenter &= center.x < xMin + width;
            isInCenter &= center.y < yMin + height;
            return isInCenter;
        }

        glm ::float2 GetCenter()
        {
            return glm::float2(xMin + width * 0.5f, yMin + height * 0.5f);
        }
    };

    struct TNode
    {
        int mip;
        int index;
        TRect rect;
        float minHeight;
        float maxHeight;
        int neighbor;

        static std::uint32_t SpecificMipLevelWidth(int curMipLevel);
        // �ض�MipLevel��Node�ڵ�����
        static std::uint32_t SpecificMipLevelNodeCount(int curMipLevel);
        // ����ָ��MipLevel��Node�ڵ�����
        static std::uint32_t ToMipLevelNodeCount(int curMipLevel);
        // ��ǰMipLevel��Index�ڵ���ȫ��Array�е�λ��
        static std::uint32_t SpecificMipLevelNodeInArrayOffset(int curMipLevel, int curLevelIndex);
    };

    #define TerrainMipLevel 9

    struct InternalTerrain;
    class RenderResource;

    class TerrainRenderHelper
    {
    public:
        TerrainRenderHelper();
        ~TerrainRenderHelper();

        void InitTerrainRenderer(SceneTerrainRenderer* sceneTerrainRenderer, InternalTerrain* internalTerrain, RenderResource* renderResource);

        void InitTerrainBasicInfo();
        void InitTerrainHeightAndNormalMap();
        void InitInternalTerrainPatchMesh();

        void InitTerrainNodePage();

        void GenerateTerrainNodes();
        TNode GenerateTerrainNodes(TRect rect, uint32_t patchIndex, int mip);

        float GetTerrainHeight(glm::float2 localXZ);
        glm::float3 GetTerrainNormal(glm::float2 localXZ);

        static InternalScratchMesh GenerateTerrainPatchScratchMesh();
        static InternalMesh GenerateTerrainPatchMesh(RenderResource* pRenderResource, InternalScratchMesh internalScratchMesh);

        std::vector<TNode> _TNodes; // ���Լ�¼���е�ͼ��

        SceneTerrainRenderer* mSceneTerrainRenderer;
        InternalTerrain* mInternalTerrain;
        RenderResource*  mRenderResource;

    private:
        InternalScratchMesh mInternalScratchMesh;
        InternalMesh mInternalhMesh;

        std::shared_ptr<MoYu::MoYuScratchImage> terrain_heightmap_scratch;
        std::shared_ptr<MoYu::MoYuScratchImage> terrain_normalmap_scratch;

        std::shared_ptr<RHI::D3D12Texture> terrain_heightmap;
        std::shared_ptr<RHI::D3D12Texture> terrain_normalmap;
    };


} // namespace MoYu
