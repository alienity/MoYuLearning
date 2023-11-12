#include "runtime/function/render/terrain_render_helper.h"
#include "runtime/function/render/render_resource.h"
#include "runtime/core/math/moyu_math2.h"
#include "runtime/core/log/log_system.h"

namespace MoYu
{
    // ÿһ��level��ÿ��patch�Ŀ��ȣ�����ÿ��level��node�ڵ�������ͬ�ģ�����8x8��
    int PatchWidthOfMipLevel[TerrainMipLevel] = {16, 32, 64, 128, 256, 512}; // 1024
    // ÿ��level��node�ڵ�������ͬ�ģ�����8x8��
    int PatchNodeWidth = 8;

    /*
    * ������ʵ��terrain��local�ռ��ڣ�����ȷ��ȷ�������Ӧ�����ص��ϵ�ÿ������terrain�ϵ�λ��
    * 1. ���������terrain�ľ���ĳ������A�ϣ�
    * 2. ȷ����A��Ϊԭ�㣬��Χһֱ��terrain��Ե������miplevel�ϵ�patch���Լ�patch�Ĵ�heightmap��mipͼ�л�ȡ���������С�߶�
    * 3. ��ȡ��ÿ��patch��node��ͬʱ�ҵ����������ҵľ�������node���ȵ�λ�ô���Ӧ��miplevel������ȷ���˻��ķ���
    * 4. ȡ��������patch��node��Ҫ����Դ�����ã��Ա������Ӱ
    * 5. ȡ��������patch��nodeҲҪ�����������
    * 
    * ������gpu�Ͼ�û����Զ����patch�Զ����mip�ȼ��ˣ�����ʵ������Ҫ��cpu��Ԥ����һ��mipƫ��ͼ�����mipƫ��ֵ�Ǹ����ݶ�����ģ�
    * ��������Ҳ����ͨ��gpu���㣬����Ҫ��ǰ��һ��mip������ֱ�Ӷ�ȫͼ�㣬��С���߶Ⱥ��ݶ�
    * 
    * ���������õ�ʱ��Ҳ����ֱ��ʹ����һ֡��depthpyramid��gpudriven����
    * 
    * ʵ�ֲ��裺
    * 0. �������λ����terrain�Ŀռ��λ�ã�ȡһ�����뵽������λ��A
    * 1. ���TerrainHeightMap��MaxHeightMap��MinHeightMap��mipmapͼ
    * 2. ������С���ֵ����һ��mipoffsetͼ��heightThresholdֵ����MaxHeight*0.00001
    * 3. ������ǰ��patchmesh�Ŀ������ŵ�2����mip0�Ͼ���8x8��patchmeshnode��
    * 4. ��A��Ϊ��㣬������Χ8��level��Ӧ������patch���浽struct{ float2 offset; float mip; float mipOffset; float minHeight; float maxHeight; } // ����mip��������µ�patch��Ӧ��scale
    * 5. ʹ�ù�Դ����ֱ��������Ե�4���õ���patch��������м���
    * 6. ������Ӱ��ֱ�ӻ��ƶ���
    */

    /**/
    uint32_t TNode::SpecificMipLevelWidth(int curMipLevel)
    {
        return 0;

        //uint32_t totalNodeCountWidth = 1 << RootLevelNodeWidthBit;
        //for (size_t i = 1; i <= curMipLevel; i++)
        //{
        //    totalNodeCountWidth = totalNodeCountWidth << SubLevelNodeWidthBit;
        //}
        //return totalNodeCountWidth;
    }

    // �ض�MipLevel��Node�ڵ�����
    uint32_t TNode::SpecificMipLevelNodeCount(int curMipLevel)
    {
        return 0;

        //uint32_t totalNodeCount = 1 << RootLevelNodeCountBit;
        //for (size_t i = 1; i <= curMipLevel; i++)
        //{
        //    totalNodeCount = totalNodeCount << SubLevelNodeCountBit;
        //}
        //return totalNodeCount;
    }

    // ����ָ��MipLevel��Node�ڵ�����
    uint32_t TNode::ToMipLevelNodeCount(int curMipLevel)
    {
        uint32_t totalNodeCount = 0;
        for (size_t i = 0; i <= curMipLevel; i++)
        {
            totalNodeCount += SpecificMipLevelNodeCount(i);
        }
        return totalNodeCount;
    }

    // ��ǰMipLevel��Index�ڵ���ȫ��Array�е�λ��
    uint32_t TNode::SpecificMipLevelNodeInArrayOffset(int curMipLevel, int curLevelIndex)
    {
        uint32_t _parentMipLevelOffset = ToMipLevelNodeCount(curMipLevel - 1);
        uint32_t _curNodeOffset        = _parentMipLevelOffset + curLevelIndex;
        return _curNodeOffset;
    }

    //---------------------------------------------------------------------------------

	TerrainRenderHelper::TerrainRenderHelper()
	{

	}

	TerrainRenderHelper::~TerrainRenderHelper()
	{

	}

    void TerrainRenderHelper::InitTerrainRenderer(SceneTerrainRenderer* sceneTerrainRenderer, InternalTerrain* internalTerrain, RenderResource* renderResource)
	{
        mSceneTerrainRenderer = sceneTerrainRenderer;
        mInternalTerrain = internalTerrain;
        mRenderResource = renderResource;

        InitTerrainBasicInfo();
        InitInternalTerrainPatchMesh();
        InitTerrainHeightAndNormalMap();
        //InitTerrainNodePage();
	}

    void TerrainRenderHelper::InitTerrainBasicInfo()
    {
        mInternalTerrain->terrain_size       = mSceneTerrainRenderer->m_scene_terrain_mesh.terrain_size;
        mInternalTerrain->terrain_max_height = mSceneTerrainRenderer->m_scene_terrain_mesh.terrian_max_height;
        //mInternalTerrain->terrain_root_patch_number =
        //    mInternalTerrain->terrain_size.x / TNode::SpecificMipLevelWidth(0);
        mInternalTerrain->terrain_mip_levels = TerrainMipLevel;
    }

    void TerrainRenderHelper::InitTerrainHeightAndNormalMap()
    {
        if (terrain_heightmap_scratch == nullptr)
        {
            terrain_heightmap_scratch = mRenderResource->loadImage(
                mSceneTerrainRenderer->m_scene_terrain_mesh.m_terrain_height_map.m_image_file);
        }
        if (terrain_normalmap_scratch == nullptr)
        {
            terrain_normalmap_scratch = mRenderResource->loadImage(
                mSceneTerrainRenderer->m_scene_terrain_mesh.m_terrain_normal_map.m_image_file);
        }
        mInternalTerrain->terrain_heightmap_scratch = terrain_heightmap_scratch;
        mInternalTerrain->terrain_normalmap_scratch = terrain_normalmap_scratch;

        if (terrain_heightmap == nullptr || terrain_normalmap == nullptr)
        {
            mRenderResource->startUploadBatch();

            if (terrain_heightmap == nullptr)
            {
                SceneImage terrainSceneImage = mSceneTerrainRenderer->m_scene_terrain_mesh.m_terrain_height_map;

                terrain_heightmap = mRenderResource->createTex2D(terrain_heightmap_scratch,
                                                                 DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM,
                                                                 terrainSceneImage.m_is_srgb,
                                                                 terrainSceneImage.m_auto_mips,
                                                                 false);
            }
            if (terrain_normalmap == nullptr)
            {
                SceneImage terrainSceneImage = mSceneTerrainRenderer->m_scene_terrain_mesh.m_terrain_normal_map;

                terrain_normalmap = mRenderResource->createTex2D(terrain_normalmap_scratch,
                                                                 DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM,
                                                                 terrainSceneImage.m_is_srgb,
                                                                 terrainSceneImage.m_auto_mips,
                                                                 false);
            }

            mRenderResource->endUploadBatch();
        }
        mInternalTerrain->terrain_heightmap = terrain_heightmap;
        mInternalTerrain->terrain_normalmap = terrain_normalmap;
    }

    void TerrainRenderHelper::InitInternalTerrainPatchMesh()
    {
        if (mInternalScratchMesh.scratch_vertex_buffer.vertex_buffer == nullptr)
        {
            mInternalScratchMesh = GenerateTerrainPatchScratchMesh();
        }
        mInternalTerrain->terrain_patch_scratch_mesh = mInternalScratchMesh;

        if (mInternalhMesh.vertex_buffer.vertex_buffer == nullptr)
        {
            mInternalhMesh = GenerateTerrainPatchMesh(mRenderResource, mInternalScratchMesh);
        }
        mInternalTerrain->terrain_patch_mesh = mInternalhMesh;
    }

	void TerrainRenderHelper::InitTerrainNodePage()
	{
        /*
		if (_tPageRoot == nullptr)
		{
            float perSize = 64;
            TRect rect = TRect {0, 0, (float)terrainSize, (float)terrainSize};
			_tPageRoot = std::make_shared<TNodePage>(rect);
            for (size_t i = rect.xMin; i < rect.xMin + rect.width; i += perSize)
            {
                for (size_t j = rect.yMin; j < rect.yMin + rect.height; j += perSize)
                {
                    _tPageRoot->children.push_back(TNodePage(TRect {(float)i, (float)j, perSize, perSize}, terrainPageLevel));
                }
            }
		}
        */

        GenerateTerrainNodes();
	}

    /*
	void TerrainRenderHelper::GenerateTerrainNodes()
	{
        uint32_t rootPageCount = terrainSize / rootPageSize;

        uint32_t pageSize = rootPageCount * (1 - glm::pow(4, terrainPageMips)) / (1 - 4);
        _TNodes.reserve(pageSize);

        std::queue<TNode> _TNodeQueue;

		TRect rect = TRect {0, 0, (float)terrainSize, (float)terrainSize};
        for (size_t j = rect.yMin, int yIndex = 0; j < rect.yMin + rect.height; j += rootPageSize, yIndex += 1)
        {
            for (size_t i = rect.xMin, int xIndex = 0; i < rect.xMin + rect.width; i += rootPageSize, xIndex += 1)
            {
                uint32_t _patchIndex = yIndex * rootPageCount + xIndex;
                TNode _TNode = GenerateTerrainNodes(TRect {(float)i, (float)j, (float)rootPageSize, (float)rootPageSize}, _patchIndex, 0);
                _TNodeQueue.push(_TNode);
            }
        }

        while (!_TNodeQueue.empty())
        {
            TNode _TNode = _TNodeQueue.front();
            _TNodeQueue.pop();

            uint32_t _nodePatchIndex = _TNode.index;
            uint32_t _nodeMipIndex = _TNode.mip;

            uint32_t _arrayOffset = TNode::InArrayOffset(rootPageCount, _nodeMipIndex, _nodePatchIndex);
            _TNodes[_arrayOffset] = _TNode;

            TRect _rect = _TNode.rect;

            if (_nodeMipIndex < terrainPageMips - 1)
            {
                TNode _TNode00 = GenerateTerrainNodes(TRect {_rect.xMin, _rect.yMin, _rect.width * 0.5f, _rect.height * 0.5f},
                    _nodePatchIndex << 2 + 0, _nodeMipIndex + 1);
                TNode _TNode10 = GenerateTerrainNodes(TRect {_rect.xMin + _rect.width * 0.5f, _rect.yMin, _rect.width * 0.5f, _rect.height * 0.5f},
                    _nodePatchIndex << 2 + 1, _nodeMipIndex + 1);
                TNode _TNode01 = GenerateTerrainNodes(TRect {_rect.xMin, _rect.yMin + _rect.height * 0.5f, _rect.width * 0.5f, _rect.height * 0.5f},
                    _nodePatchIndex << 2 + 2, _nodeMipIndex + 1);
                TNode _TNode11 = GenerateTerrainNodes(TRect {_rect.xMin + _rect.width * 0.5f, _rect.yMin + _rect.height * 0.5f, _rect.width * 0.5f, _rect.height * 0.5f},
                    _nodePatchIndex << 2 + 3, _nodeMipIndex + 1);

                _TNodeQueue.push(_TNode00);
                _TNodeQueue.push(_TNode10);
                _TNodeQueue.push(_TNode01);
                _TNodeQueue.push(_TNode11);
            }
        }
	}
    */

    void TerrainRenderHelper::GenerateTerrainNodes()
    {
        int terrainSize = mInternalTerrain->terrain_size.x;
        //int terrain_root_patch_number = mInternalTerrain->terrain_root_patch_number;

        float rootNodeWidth = (float)MoYu::TNode::SpecificMipLevelWidth(0);
        float rootNodeCount = terrainSize / rootNodeWidth;

        uint32_t _totalNodeCount = TNode::ToMipLevelNodeCount(TerrainMipLevel);
        _TNodes.resize(_totalNodeCount);

        int yIndex = 0;
        int xIndex = 0;
        TRect rect = TRect {0, 0, (float)terrainSize, (float)terrainSize};
        for (size_t j = rect.yMin; j < rect.yMin + rect.height; j += rootNodeWidth, yIndex += 1)
        {
            for (size_t i = rect.xMin; i < rect.xMin + rect.width; i += rootNodeWidth, xIndex += 1)
            {
                uint32_t _patchIndex = yIndex * rootNodeCount + xIndex;
                TNode _tNode = GenerateTerrainNodes(TRect {(float)i, (float)j, (float)rootNodeWidth, (float)rootNodeWidth}, _patchIndex, 0);
                uint32_t _arrayOffset = TNode::SpecificMipLevelNodeInArrayOffset(0, _patchIndex);
                _TNodes[_arrayOffset] = _tNode;
            }
        }

        std::wstring _logStr = fmt::format(L"_TNodes count = {}", _TNodes.size());
        MoYu::LogSystem::Instance()->log(MoYu::LogSystem::LogLevel::Info, _logStr);
    }

	TNode TerrainRenderHelper::GenerateTerrainNodes(TRect rect, uint32_t pathcIndex, int mip)
	{

        float _minHeight = std::numeric_limits<float>::max();
        float _maxHeight = -std::numeric_limits<float>::max();

        if (mip < TerrainMipLevel - 1)
        {
            uint32_t _offset = 1 << (2 * pathcIndex);

            TNode _TNode00 = GenerateTerrainNodes(TRect {rect.xMin, rect.yMin, rect.width * 0.5f, rect.height * 0.5f}, _offset + 0, mip + 1);
            TNode _TNode01 = GenerateTerrainNodes(TRect {rect.xMin + rect.width * 0.5f, rect.yMin, rect.width * 0.5f, rect.height * 0.5f}, _offset + 1, mip + 1);
            TNode _TNode10 = GenerateTerrainNodes(TRect {rect.xMin, rect.yMin + rect.height * 0.5f, rect.width * 0.5f, rect.height * 0.5f}, _offset + 2, mip + 1);
            TNode _TNode11 = GenerateTerrainNodes(TRect {rect.xMin + rect.width * 0.5f, rect.yMin + rect.height * 0.5f, rect.width * 0.5f, rect.height * 0.5f}, _offset + 3, mip + 1);

            _TNodes[MoYu::TNode::SpecificMipLevelNodeInArrayOffset(_TNode00.mip, _TNode00.index)] = _TNode00;
            _TNodes[MoYu::TNode::SpecificMipLevelNodeInArrayOffset(_TNode01.mip, _TNode01.index)] = _TNode01;
            _TNodes[MoYu::TNode::SpecificMipLevelNodeInArrayOffset(_TNode10.mip, _TNode10.index)] = _TNode10;
            _TNodes[MoYu::TNode::SpecificMipLevelNodeInArrayOffset(_TNode11.mip, _TNode11.index)] = _TNode11;

            _minHeight = glm::min(glm::min(_TNode00.minHeight, _TNode01.minHeight), glm::min(_TNode10.minHeight, _TNode11.minHeight));
            _maxHeight = glm::max(glm::max(_TNode00.maxHeight, _TNode01.maxHeight), glm::max(_TNode10.maxHeight, _TNode11.maxHeight));
        }
        else if (mip == TerrainMipLevel - 1)
        {
            for (float i = rect.xMin; i < rect.xMin + rect.width; i += 1.0f)
            {
                for (float j = rect.yMin; j < rect.yMin + rect.height; j += 1.0f)
                {
                    float _curHeight = GetTerrainHeight(glm::float2(i, j));
                    _minHeight = glm::min(_minHeight, _curHeight);
                    _maxHeight = glm::max(_maxHeight, _curHeight);
                }
            }
        }
        
        TNode _TNode     = {};
        _TNode.mip       = mip;
        _TNode.index     = pathcIndex;
        _TNode.rect      = rect;
        _TNode.minHeight = _maxHeight;
        _TNode.maxHeight = _minHeight;

        return _TNode;
	}

	float TerrainRenderHelper::GetTerrainHeight(glm::float2 localXZ)
	{
        auto _terrain_height_map = mInternalTerrain->terrain_heightmap_scratch;

		auto _heightMetaData = _terrain_height_map->GetMetadata();
        auto _heightMap = _terrain_height_map->GetImage(0, 0, 0);

		if (localXZ.x < 0 || localXZ.y < 0 || localXZ.x >= _heightMetaData.width - 1 || localXZ.y >= _heightMetaData.height - 1)
            return -1;

		uint32_t x0 = glm::floor(localXZ.x);
		uint32_t x1 = glm::ceil(localXZ.x);
        uint32_t z0 = glm::floor(localXZ.y);
        uint32_t z1 = glm::ceil(localXZ.y);

        float xfrac = glm::fract(localXZ.x);
        float zfrac = glm::fract(localXZ.y);

		float _x0z0Height = (*(glm::uvec4*)(_heightMap->pixels + (uint32_t)(sizeof(uint8_t) * 4 * (x0 + z0 * _heightMetaData.width)))).z;
		float _x1z0Height = (*(glm::uvec4*)(_heightMap->pixels + (uint32_t)(sizeof(uint8_t) * 4 * (x1 + z0 * _heightMetaData.width)))).z;
		float _x0z1Height = (*(glm::uvec4*)(_heightMap->pixels + (uint32_t)(sizeof(uint8_t) * 4 * (x0 + z1 * _heightMetaData.width)))).z;
		float _x1z1Height = (*(glm::uvec4*)(_heightMap->pixels + (uint32_t)(sizeof(uint8_t) * 4 * (x1 + z1 * _heightMetaData.width)))).z;

		float _Height = glm::lerp(glm::lerp(_x0z0Height, _x1z0Height, xfrac), glm::lerp(_x0z1Height, _x1z1Height, xfrac), zfrac);

		return _Height;
	}

	glm::float3 TerrainRenderHelper::GetTerrainNormal(glm::float2 localXZ)
	{
        auto _terrain_normal_map = mInternalTerrain->terrain_normalmap_scratch;

		auto _normalMetaData = _terrain_normal_map->GetMetadata();
        auto _normalMap = _terrain_normal_map->GetImage(0, 0, 0);

		if (localXZ.x < 0 || localXZ.y < 0 || localXZ.x >= _normalMetaData.width - 1 || localXZ.y >= _normalMetaData.height - 1)
            return glm::float3(-1);

		uint32_t x0 = glm::floor(localXZ.x);
		uint32_t x1 = glm::ceil(localXZ.x);
        uint32_t z0 = glm::floor(localXZ.y);
        uint32_t z1 = glm::ceil(localXZ.y);

        float xfrac = glm::fract(localXZ.x);
        float zfrac = glm::fract(localXZ.y);

		glm::uvec3 _x0z0NormalU = (*(glm::uvec4*)(_normalMap->pixels + (uint32_t)(sizeof(uint8_t) * 4 * (x0 + z0 * _normalMetaData.width))));
		glm::uvec3 _x1z0NormalU = (*(glm::uvec4*)(_normalMap->pixels + (uint32_t)(sizeof(uint8_t) * 4 * (x1 + z0 * _normalMetaData.width))));
		glm::uvec3 _x0z1NormalU = (*(glm::uvec4*)(_normalMap->pixels + (uint32_t)(sizeof(uint8_t) * 4 * (x0 + z1 * _normalMetaData.width))));
		glm::uvec3 _x1z1NormalU = (*(glm::uvec4*)(_normalMap->pixels + (uint32_t)(sizeof(uint8_t) * 4 * (x1 + z1 * _normalMetaData.width))));

		glm::float3 _x0z0Normal = glm::float3(_x0z0NormalU.x, _x0z0NormalU.y, _x0z0NormalU.z);
        glm::float3 _x1z0Normal = glm::float3(_x1z0NormalU.x, _x1z0NormalU.y, _x1z0NormalU.z);
        glm::float3 _x0z1Normal = glm::float3(_x0z1NormalU.x, _x0z1NormalU.y, _x0z1NormalU.z);
        glm::float3 _x1z1Normal = glm::float3(_x1z1NormalU.x, _x1z1NormalU.y, _x1z1NormalU.z);

		glm::float3 _Normal = glm::lerp(glm::lerp(_x0z0Normal, _x1z0Normal, xfrac), glm::lerp(_x0z1Normal, _x1z1Normal, xfrac), zfrac);

		return _Normal;
	}

    D3D12TerrainPatch CreatePatch(glm::float3 position, glm::float2 coord, glm::float4 color)
    {
        D3D12TerrainPatch _t;
        _t.position  = position;
        _t.texcoord0 = coord;
        _t.color     = color;
        _t.normal    = glm::vec3(0, 1, 0);
        _t.tangent   = glm::vec4(1, 0, 0, 1);
        return _t;
    }

    InternalScratchMesh TerrainRenderHelper::GenerateTerrainPatchScratchMesh()
    {
        std::vector<D3D12TerrainPatch> vertices {};
        std::vector<int> indices {};

        float patchScale = 2.0f;

        vertices.push_back(CreatePatch(patchScale * glm::vec3(0.0, 0, 0.0), glm::vec2(0, 0), glm::vec4(0, 0, 0, 0)));
        vertices.push_back(CreatePatch(patchScale * glm::vec3(0.25, 0, 0.0), glm::vec2(0.25, 0), glm::vec4(1, 0, 0, 0)));
        vertices.push_back(CreatePatch(patchScale * glm::vec3(0.5, 0, 0.0), glm::vec2(0.5, 0), glm::vec4(0, 0, 0, 0)));
        vertices.push_back(CreatePatch(patchScale * glm::vec3(0.75, 0, 0.0), glm::vec2(0.75, 0), glm::vec4(1, 0, 0, 0)));
        vertices.push_back(CreatePatch(patchScale * glm::vec3(1.0, 0, 0.0), glm::vec2(1.0, 0), glm::vec4(0, 0, 0, 0)));

        vertices.push_back(CreatePatch(patchScale * glm::vec3(0.0, 0, 0.25),  glm::vec2(0, 0.25), glm::vec4(0, 0, 1, 0)));
        vertices.push_back(CreatePatch(patchScale * glm::vec3(0.25, 0, 0.25), glm::vec2(0.25, 0.25), glm::vec4(0, 0, 0, 0)));
        vertices.push_back(CreatePatch(patchScale * glm::vec3(0.5, 0, 0.25), glm::vec2(0.5, 0.25), glm::vec4(0, 0, 0, 0)));
        vertices.push_back(CreatePatch(patchScale * glm::vec3(0.75, 0, 0.25), glm::vec2(0.75, 0.25), glm::vec4(0, 0, 0, 0)));
        vertices.push_back(CreatePatch(patchScale * glm::vec3(1.0, 0, 0.25), glm::vec2(1.0, 0.25), glm::vec4(0, 0, 0, 1)));

        vertices.push_back(CreatePatch(patchScale * glm::vec3(0.0, 0, 0.5), glm::vec2(0, 0.5), glm::vec4(0, 0, 0, 0)));
        vertices.push_back(CreatePatch(patchScale * glm::vec3(0.25, 0, 0.5), glm::vec2(0.25, 0.5), glm::vec4(0, 0, 0, 0)));
        vertices.push_back(CreatePatch(patchScale * glm::vec3(0.5, 0, 0.5), glm::vec2(0.5, 0.5), glm::vec4(0, 0, 0, 0)));
        vertices.push_back(CreatePatch(patchScale * glm::vec3(0.75, 0, 0.5), glm::vec2(0.75, 0.5), glm::vec4(0, 0, 0, 0)));
        vertices.push_back(CreatePatch(patchScale * glm::vec3(1.0, 0, 0.5), glm::vec2(1.0, 0.5), glm::vec4(0, 0, 0, 0)));

        vertices.push_back(CreatePatch(patchScale * glm::vec3(0.0, 0, 0.75),  glm::vec2(0, 0.75), glm::vec4(0, 0, 1, 0)));
        vertices.push_back(CreatePatch(patchScale * glm::vec3(0.25, 0, 0.75),  glm::vec2(0.25, 0.75), glm::vec4(0, 0, 0, 0)));
        vertices.push_back(CreatePatch(patchScale * glm::vec3(0.5, 0, 0.75),  glm::vec2(0.5, 0.75), glm::vec4(0, 0, 0, 0)));
        vertices.push_back(CreatePatch(patchScale * glm::vec3(0.75, 0, 0.75),  glm::vec2(0.75, 0.75), glm::vec4(0, 0, 0, 0)));
        vertices.push_back(CreatePatch(patchScale * glm::vec3(1.0, 0, 0.75),  glm::vec2(1.0, 0.75), glm::vec4(0, 0, 0, 1)));

        vertices.push_back(CreatePatch(patchScale * glm::vec3(0.0, 0, 1.0),   glm::vec2(0, 1.0), glm::vec4(0, 0, 0, 0)));
        vertices.push_back(CreatePatch(patchScale * glm::vec3(0.25, 0, 1.0),   glm::vec2(0.25, 1.0), glm::vec4(0, 1, 0, 0)));
        vertices.push_back(CreatePatch(patchScale * glm::vec3(0.5, 0, 1.0),   glm::vec2(0.5, 1.0), glm::vec4(0, 0, 0, 0)));
        vertices.push_back(CreatePatch(patchScale * glm::vec3(0.75, 0, 1.0),   glm::vec2(0.75, 1.0), glm::vec4(0, 1, 0, 0)));
        vertices.push_back(CreatePatch(patchScale * glm::vec3(1.0, 0, 1.0),   glm::vec2(1.0, 1.0), glm::vec4(0, 0, 0, 0)));

        #define AddTriIndices(a, b, c) indices.push_back(a); indices.push_back(b); indices.push_back(c);

        #define AddQuadIndices(d) AddTriIndices(0 + d, 5 + d, 6 + d) AddTriIndices(0 + d, 6 + d, 1 + d)

        #define AddQuadIndices4(d) AddQuadIndices(d) AddQuadIndices(d + 1) AddQuadIndices(d + 2) AddQuadIndices(d + 3)

        #define AddQuadIndices16() AddQuadIndices4(0) AddQuadIndices4(5) AddQuadIndices4(10) AddQuadIndices4(15) AddQuadIndices4(20)

        AddQuadIndices16()

        std::uint32_t vertex_buffer_size = vertices.size() * sizeof(D3D12TerrainPatch);
        InternalScratchVertexBuffer scratch_vertex_buffer {};
        scratch_vertex_buffer.input_element_definition = D3D12TerrainPatch::InputElementDefinition;
        scratch_vertex_buffer.vertex_count = vertices.size();
        scratch_vertex_buffer.vertex_buffer = std::make_shared<MoYuScratchBuffer>();
        scratch_vertex_buffer.vertex_buffer->Initialize(vertex_buffer_size);
        memcpy(scratch_vertex_buffer.vertex_buffer->GetBufferPointer(), vertices.data(), vertex_buffer_size);

        std::uint32_t index_buffer_size = indices.size() * sizeof(std::uint32_t);
        InternalScratchIndexBuffer scratch_index_buffer {};
        scratch_index_buffer.index_stride = sizeof(uint32_t);
        scratch_index_buffer.index_count  = indices.size();
        scratch_index_buffer.index_buffer = std::make_shared<MoYuScratchBuffer>();
        scratch_index_buffer.index_buffer->Initialize(index_buffer_size);
        memcpy(scratch_index_buffer.index_buffer->GetBufferPointer(), indices.data(), index_buffer_size);

        InternalScratchMesh scratchMesh {scratch_index_buffer, scratch_vertex_buffer};

        return scratchMesh;
    }

    InternalMesh TerrainRenderHelper::GenerateTerrainPatchMesh(RenderResource* pRenderResource, InternalScratchMesh internalScratchMesh)
    {
        InternalVertexBuffer vertex_buffer = pRenderResource->createVertexBuffer<D3D12TerrainPatch>(
            internalScratchMesh.scratch_vertex_buffer.input_element_definition,
            internalScratchMesh.scratch_vertex_buffer.vertex_buffer);

        InternalIndexBuffer index_buffer =
            pRenderResource->createIndexBuffer<uint32_t>(internalScratchMesh.scratch_index_buffer.index_buffer);

        InternalMesh internalhMesh {false, {}, index_buffer, vertex_buffer};

        return internalhMesh;
    }

}