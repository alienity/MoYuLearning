﻿#include "runtime/function/render/renderer/terrain_cull_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/core/math/moyu_math2.h"
#include "runtime/function/render/rhi/d3d12/d3d12_graphicsCommon.h"
#include "runtime/function/render/terrain_render_helper.h"
#include "runtime/function/render/renderer/pass_helper.h"
#include "fmt/core.h"
#include <cassert>

namespace MoYu
{
    void IndirectTerrainCullPass::initialize(const TerrainCullInitInfo& init_info)
    {
        ShaderCompiler* m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        //--------------------------------------------------------------------------
        {
            ShaderCompileOptions shaderCompileOpt = ShaderCompileOptions(L"InitQuadTree");
            shaderCompileOpt.SetDefine(L"INIT_QUAD_TREE", L"1");

            InitQuadTreeCS = m_ShaderCompiler->CompileShader(
                RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Tools/Terrain/TerrainBuildCS.hlsl", shaderCompileOpt);

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                .Add32BitConstants<0, 0>(16)
                .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                .AllowInputLayout()
                .AllowResourceDescriptorHeapIndexing()
                .AllowSampleDescriptorHeapIndexing();

            pInitQuadTreeSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pInitQuadTreeSignature.get());
            psoStream.CS = &InitQuadTreeCS;
            PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };

            pInitQuadTreePSO =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"InitQuadTreePSO", psoDesc);
        }
        //--------------------------------------------------------------------------

        //--------------------------------------------------------------------------
        {
            ShaderCompileOptions shaderCompileOpt = ShaderCompileOptions(L"TraverseQuadTree");
            shaderCompileOpt.SetDefine(L"TRAVERSE_QUAD_TREE", L"1");

            TraverseQuadTreeCS = m_ShaderCompiler->CompileShader(
                RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Tools/Terrain/TerrainBuildCS.hlsl", shaderCompileOpt);

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                .Add32BitConstants<0, 0>(16)
                .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                .AllowInputLayout()
                .AllowResourceDescriptorHeapIndexing()
                .AllowSampleDescriptorHeapIndexing();

            pTraverseQuadTreeSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pTraverseQuadTreeSignature.get());
            psoStream.CS = &TraverseQuadTreeCS;
            PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };

            pTraverseQuadTreePSO =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"TraverseQuadTreePSO", psoDesc);
        }
        //--------------------------------------------------------------------------

        //--------------------------------------------------------------------------
        {
            ShaderCompileOptions shaderCompileOpt = ShaderCompileOptions(L"BuildLodMap");
            shaderCompileOpt.SetDefine(L"BUILD_LODMAP", L"1");

            BuildLodMapCS = m_ShaderCompiler->CompileShader(
                RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Tools/Terrain/TerrainBuildCS.hlsl", shaderCompileOpt);

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                .Add32BitConstants<0, 0>(16)
                .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                .AllowInputLayout()
                .AllowResourceDescriptorHeapIndexing()
                .AllowSampleDescriptorHeapIndexing();

            pBuildLodMapSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pBuildLodMapSignature.get());
            psoStream.CS = &BuildLodMapCS;
            PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };

            pBuildLodMapPSO =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"BuildLodMapPSO", psoDesc);
        }

        //--------------------------------------------------------------------------

        //--------------------------------------------------------------------------
        {
            ShaderCompileOptions shaderCompileOpt = ShaderCompileOptions(L"BuildPatches");
            shaderCompileOpt.SetDefine(L"BUILD_PATCHES", L"1");
            shaderCompileOpt.SetDefine(L"ENABLE_FRUS_CULL", L"1");

            BuildPatchesCS = m_ShaderCompiler->CompileShader(
                RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Tools/Terrain/TerrainBuildCS.hlsl", shaderCompileOpt);

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                .Add32BitConstants<0, 0>(16)
                .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                .AllowInputLayout()
                .AllowResourceDescriptorHeapIndexing()
                .AllowSampleDescriptorHeapIndexing();

            pBuildPatchesSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pBuildPatchesSignature.get());
            psoStream.CS = &BuildPatchesCS;
            PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };

            pBuildPatchesPSO =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"BuildPatchesPSO", psoDesc);
        }

        //--------------------------------------------------------------------------

        iMinMaxHeightReady = false;
    }

    void IndirectTerrainCullPass::prepareMeshData(std::shared_ptr<RenderResource> render_resource)
    {
        if (pMinHeightMap == nullptr || pMaxHeightMap == nullptr)
        {
            InternalTerrainRenderer& internalTerrainRenderer = m_render_scene->m_terrain_renderers[0].internalTerrainRenderer;

            std::shared_ptr<RHI::D3D12Texture> terrainHeightmap = internalTerrainRenderer.ref_terrain.terrain_heightmap;
            auto heightmapDesc = terrainHeightmap->GetDesc();

            int mipLevel = glm::log2(glm::fmin((float)heightmapDesc.Width, heightmapDesc.Height));

            pMinHeightMap =
                RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                    heightmapDesc.Width,
                    heightmapDesc.Height,
                    mipLevel,
                    heightmapDesc.Format,
                    RHI::RHISurfaceCreateRandomWrite | RHI::RHISurfaceCreateMipmap,
                    1,
                    L"TerrainMinHeightmap",
                    D3D12_RESOURCE_STATE_COMMON,
                    std::nullopt);

            pMaxHeightMap =
                RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                    heightmapDesc.Width,
                    heightmapDesc.Height,
                    mipLevel,
                    heightmapDesc.Format,
                    RHI::RHISurfaceCreateRandomWrite | RHI::RHISurfaceCreateMipmap,
                    1,
                    L"TerrainMaxHeightmap",
                    D3D12_RESOURCE_STATE_COMMON,
                    std::nullopt);
        }

        if (pLodMap == nullptr)
        {
            pLodMap =
                RHI::D3D12Texture::Create2D(
                    m_Device->GetLinkedDevice(),
                    160,
                    160,
                    1,
                    DXGI_FORMAT_R8_SNORM,
                    RHI::RHISurfaceCreateRandomWrite,
                    1,
                    L"LodMap",
                    D3D12_RESOURCE_STATE_COMMON,
                    std::nullopt);

        }

        if (mTerrainConsBuffer == nullptr)
        {
            mTerrainConsBuffer =
                RHI::D3D12Buffer::Create(
                    m_Device->GetLinkedDevice(),
                    RHI::RHIBufferTargetNone,
                    1,
                    MoYu::AlignUp(sizeof(TerrainConsBuffer), 256),
                    L"TerrainConsBuffer",
                    RHI::RHIBufferModeDynamic,
                    D3D12_RESOURCE_STATE_GENERIC_READ);
        }

        HLSL::FrameUniforms& frameUniform = render_resource->m_FrameUniforms;
        glm::float3 cameraPosition = frameUniform.cameraUniform._WorldSpaceCameraPos;
        glm::float4x4 cameraViewProj = frameUniform.cameraUniform._ViewProjMatrix;

        InternalTerrainRenderer& internalTerrainRenderer = m_render_scene->m_terrain_renderers[0].internalTerrainRenderer;
        glm::float3 terrainSize = internalTerrainRenderer.ref_terrain.terrain_size;;

        int nodeCount = MAX_LOD_NODE_COUNT;
        for (int lod = MAX_TERRAIN_LOD; lod >= 0; lod--)
        {
            int nodeSize = terrainSize.x / nodeCount;
            int patchExtent = nodeSize / PATCH_MESH_GRID_COUNT;
            int sectorCountPerNode = (int)glm::pow(2, lod);
            worldLODParams[lod] = glm::float4(nodeSize, patchExtent, nodeCount, sectorCountPerNode);
            nodeCount *= 2;
        }


        int nodeIdOffset = 0;
        for (int lod = MAX_TERRAIN_LOD; lod >= 0; lod--)
        {
            nodeIDOffsetLOD[lod] = nodeIdOffset;
            nodeIdOffset += (int)(worldLODParams[lod].z * worldLODParams[lod].z);
        }

        int maxNodeCount = nodeIdOffset;

        TerrainConsBuffer TerrainCB;
        TerrainCB.TerrainOffsetWS = glm::float3(0, 0, 0);
        TerrainCB.CameraViewProj = cameraViewProj;
        TerrainCB.CameraPositionWS = cameraPosition;
        TerrainCB.BoundsHeightRedundance = 0.1f;
        TerrainCB.WorldSize = terrainSize;
        TerrainCB.NodeEvaluationC = glm::float4(1, 0, 0, 0);
        memcpy(TerrainCB.WorldLodParams, worldLODParams, sizeof(glm::float4) * (MAX_TERRAIN_LOD + 1));
        memcpy(TerrainCB.NodeIDOffsetOfLOD, nodeIDOffsetLOD, sizeof(uint32_t) * (MAX_TERRAIN_LOD + 1));

        memcpy(mTerrainConsBuffer->GetCpuVirtualAddress<TerrainConsBuffer>(), &TerrainCB, sizeof(TerrainConsBuffer));

        //--------------------------------------------------------------------------

        if (TempNodeList[0] == nullptr)
        {
            for (int i = 0; i < 2; i++)
            {
                TempNodeList[i] = RHI::D3D12Buffer::Create(
                    m_Device->GetLinkedDevice(),
                    RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter,
                    1024,
                    sizeof(glm::uvec2),
                    fmt::format(L"TempNodeList_{}", i));
            }
            FinalNodeList = RHI::D3D12Buffer::Create(
                m_Device->GetLinkedDevice(),
                RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter,
                1024,
                sizeof(glm::uvec3),
                L"FinalNodeList");

            NodeDescriptors = RHI::D3D12Buffer::Create(
                m_Device->GetLinkedDevice(),
                RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter,
                maxNodeCount,
                sizeof(uint32_t),
                L"NodeDescriptors");
        }

        if (CulledPatchListBuffer == nullptr)
        {
            CulledPatchListBuffer = RHI::D3D12Buffer::Create(
                m_Device->GetLinkedDevice(),
                RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter,
                maxNodeCount,
                sizeof(HLSL::TerrainRenderPatch),
                L"CulledPatchListBuffer");
        }

        //--------------------------------------------------------------------------

        traverseDispatchArgsBufferDesc =
            RHI::RgBufferDesc("TraverseDisaptchArgs")
            .SetSize(1, sizeof(D3D12_DISPATCH_ARGUMENTS))
            .SetRHIBufferMode(RHI::RHIBufferMode::RHIBufferModeImmutable)
            .SetRHIBufferTarget(RHI::RHIBufferTarget::RHIBufferTargetIndirectArgs | RHI::RHIBufferTarget::RHIBufferRandomReadWrite | RHI::RHIBufferTarget::RHIBufferTargetRaw);

        buildPatchArgsBufferDesc =
            RHI::RgBufferDesc("BuildPatchArgs")
            .SetSize(1, sizeof(D3D12_DISPATCH_ARGUMENTS))
            .SetRHIBufferMode(RHI::RHIBufferMode::RHIBufferModeImmutable)
            .SetRHIBufferTarget(RHI::RHIBufferTarget::RHIBufferTargetIndirectArgs | RHI::RHIBufferTarget::RHIBufferRandomReadWrite | RHI::RHIBufferTarget::RHIBufferTargetRaw);
    }


    void IndirectTerrainCullPass::update(RHI::RenderGraph& graph, TerrainCullInput& passInput, TerrainCullOutput& passOutput)
    {
        InternalTerrainRenderer& internalTerrainRenderer = m_render_scene->m_terrain_renderers[0].internalTerrainRenderer;
        std::shared_ptr<RHI::D3D12Texture> terrainHeightmap = internalTerrainRenderer.ref_terrain.terrain_heightmap;
        std::shared_ptr<RHI::D3D12Texture> terrainNormalmap = internalTerrainRenderer.ref_terrain.terrain_normalmap;

        RHI::RgResourceHandle terrainHeightmapHandle = GImport(graph, terrainHeightmap.get());
        RHI::RgResourceHandle terrainNormalmapHandle = GImport(graph, terrainNormalmap.get());

        RHI::RgResourceHandle terrainMinHeightHandle = GImport(graph, pMinHeightMap.get());
        RHI::RgResourceHandle terrainMaxHeightHandle = GImport(graph, pMaxHeightMap.get());

        if (!iMinMaxHeightReady)
        {
            iMinMaxHeightReady = true;

            RHI::RenderPass& genHeightMapMipMapPass = graph.AddRenderPass("GenerateHeightMapMipMapPass");

            genHeightMapMipMapPass.Read(terrainHeightmapHandle, true);
            genHeightMapMipMapPass.Read(terrainMinHeightHandle, true);
            genHeightMapMipMapPass.Read(terrainMaxHeightHandle, true);
            genHeightMapMipMapPass.Write(terrainMinHeightHandle, true);
            genHeightMapMipMapPass.Write(terrainMaxHeightHandle, true);

            genHeightMapMipMapPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

                {
                    pContext->TransitionBarrier(RegGetTex(terrainHeightmapHandle), D3D12_RESOURCE_STATE_COPY_SOURCE);
                    pContext->TransitionBarrier(RegGetTex(terrainMinHeightHandle), D3D12_RESOURCE_STATE_COPY_DEST, 0);
                    pContext->TransitionBarrier(RegGetTex(terrainMaxHeightHandle), D3D12_RESOURCE_STATE_COPY_DEST, 0);
                    pContext->FlushResourceBarriers();

                    const CD3DX12_TEXTURE_COPY_LOCATION src(terrainHeightmap->GetResource(), 0);

                    const CD3DX12_TEXTURE_COPY_LOCATION dst_min(RegGetTex(terrainMinHeightHandle)->GetResource(), 0);
                    context->GetGraphicsCommandList()->CopyTextureRegion(&dst_min, 0, 0, 0, &src, nullptr);

                    const CD3DX12_TEXTURE_COPY_LOCATION dst_max(RegGetTex(terrainMaxHeightHandle)->GetResource(), 0);
                    context->GetGraphicsCommandList()->CopyTextureRegion(&dst_max, 0, 0, 0, &src, nullptr);

                    context->FlushResourceBarriers();
                }

                //--------------------------------------------------
                // Generate MinHeightMap
                //--------------------------------------------------
                {
                    RHI::D3D12Texture* _SrcTexture = RegGetTex(terrainMinHeightHandle);
                    generateMipmapForTerrainHeightmap(pContext, _SrcTexture, true);
                }

                //--------------------------------------------------
                // Generate MaxHeightMap
                //--------------------------------------------------
                {
                    RHI::D3D12Texture* _SrcTexture = RegGetTex(terrainMaxHeightHandle);
                    generateMipmapForTerrainHeightmap(pContext, _SrcTexture, false);
                }

                });
        }

        //------------------------------------------------------------------------------------

        RHI::RgResourceHandle traverseDispatchArgsHandle = graph.Create<RHI::D3D12Buffer>(traverseDispatchArgsBufferDesc);

        RHI::RgResourceHandle TempNodeListHandle[2];
        TempNodeListHandle[0] = GImport(graph, TempNodeList[0].get());
        TempNodeListHandle[1] = GImport(graph, TempNodeList[1].get());
        RHI::RgResourceHandle FinalNodeListHandle = GImport(graph, FinalNodeList.get());
        RHI::RgResourceHandle NodeDescriptorsHandle = GImport(graph, NodeDescriptors.get());

        RHI::RgResourceHandle terrainConsBufferHandle = GImport(graph, mTerrainConsBuffer.get());

        RHI::RenderPass& traverseQuadTreePass = graph.AddRenderPass("TraverseQuadTreePass");

        traverseQuadTreePass.Read(terrainConsBufferHandle);
        traverseQuadTreePass.Read(terrainMinHeightHandle);
        traverseQuadTreePass.Read(terrainMaxHeightHandle);
        traverseQuadTreePass.Read(TempNodeListHandle[0], true);
        traverseQuadTreePass.Read(TempNodeListHandle[1], true);

        traverseQuadTreePass.Write(TempNodeListHandle[0], true);
        traverseQuadTreePass.Write(TempNodeListHandle[1], true);
        traverseQuadTreePass.Write(FinalNodeListHandle, true);
        traverseQuadTreePass.Write(NodeDescriptorsHandle, true);

        traverseQuadTreePass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBufCounter(TempNodeListHandle[0]), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->TransitionBarrier(RegGetBufCounter(TempNodeListHandle[1]), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->TransitionBarrier(RegGetBufCounter(FinalNodeListHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->TransitionBarrier(RegGetBufCounter(NodeDescriptorsHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

            pContext->FlushResourceBarriers();

            pContext->ResetCounter(RegGetBufCounter(TempNodeListHandle[0]));
            pContext->ResetCounter(RegGetBufCounter(TempNodeListHandle[1]));
            pContext->ResetCounter(RegGetBufCounter(FinalNodeListHandle));
            pContext->ResetCounter(RegGetBufCounter(NodeDescriptorsHandle));

            int index = 0;

            //****************************************************************************
            pContext->TransitionBarrier(RegGetBuf(TempNodeListHandle[0]), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBufCounter(TempNodeListHandle[0]), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBuf(TempNodeListHandle[1]), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBufCounter(TempNodeListHandle[1]), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBuf(FinalNodeListHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBufCounter(FinalNodeListHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBuf(NodeDescriptorsHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBufCounter(NodeDescriptorsHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->FlushResourceBarriers();

            RHI::RgResourceHandle AppendNodeListHandle = TempNodeListHandle[index];

            RHI::D3D12UnorderedAccessView* AppendNodeListUAV = registry->GetD3D12Buffer(AppendNodeListHandle)->GetDefaultUAV().get();

            pContext->SetRootSignature(pInitQuadTreeSignature.get());
            pContext->SetPipelineState(pInitQuadTreePSO.get());
            pContext->SetConstant(0, 0, AppendNodeListUAV->GetIndex());
            pContext->Dispatch(1, 1, 1);

            //****************************************************************************
            for (int i = MAX_TERRAIN_LOD; i >= 0; i--)
            {
                pContext->TransitionBarrier(RegGetBufCounter(AppendNodeListHandle), D3D12_RESOURCE_STATE_COPY_SOURCE);
                pContext->TransitionBarrier(RegGetBuf(traverseDispatchArgsHandle), D3D12_RESOURCE_STATE_COPY_DEST);
                pContext->CopyBufferRegion(RegGetBuf(traverseDispatchArgsHandle), 0, RegGetBufCounter(AppendNodeListHandle), 0, sizeof(uint32_t));
                pContext->FillBuffer(RegGetBuf(traverseDispatchArgsHandle), sizeof(uint32_t), 1, sizeof(uint32_t) * 2);

                RHI::RgResourceHandle ConsumeNodeListHandle = TempNodeListHandle[index % 2];
                AppendNodeListHandle = TempNodeListHandle[(index + 1) % 2];

                pContext->TransitionBarrier(RegGetBuf(ConsumeNodeListHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                pContext->TransitionBarrier(RegGetBufCounter(ConsumeNodeListHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                pContext->TransitionBarrier(RegGetBuf(AppendNodeListHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                pContext->TransitionBarrier(RegGetBufCounter(AppendNodeListHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                pContext->TransitionBarrier(RegGetBuf(traverseDispatchArgsHandle), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
                pContext->FlushResourceBarriers();

                pContext->SetRootSignature(pTraverseQuadTreeSignature.get());
                pContext->SetPipelineState(pTraverseQuadTreePSO.get());

                struct RootIndexBuffer
                {
                    uint32_t consBufferIndex;
                    uint32_t minHeightmapIndex;
                    uint32_t maxHeightmapIndex;
                    uint32_t consumeNodeListBufferIndex;
                    uint32_t appendNodeListBufferIndex;
                    uint32_t appendFinalNodeListBufferIndex;
                    uint32_t nodeDescriptorsBufferIndex;
                    uint32_t PassLOD; //表示TraverseQuadTree kernel执行的LOD级别
                };

                RootIndexBuffer rootIndexBuffer = RootIndexBuffer{ RegGetBufDefCBVIdx(terrainConsBufferHandle),
                                                                   RegGetTexDefSRVIdx(terrainMinHeightHandle),
                                                                   RegGetTexDefSRVIdx(terrainMaxHeightHandle),
                                                                   RegGetBufDefUAVIdx(ConsumeNodeListHandle),
                                                                   RegGetBufDefUAVIdx(AppendNodeListHandle),
                                                                   RegGetBufDefUAVIdx(FinalNodeListHandle),
                                                                   RegGetBufDefUAVIdx(NodeDescriptorsHandle),
                                                                   (uint32_t)i };

                pContext->SetConstantArray(0, sizeof(rootIndexBuffer) / sizeof(uint32_t), &rootIndexBuffer);

                pContext->DispatchIndirect(RegGetBuf(traverseDispatchArgsHandle), 0);

                index += 1;
            }
            });

        //------------------------------------------------------------------------------------

        RHI::RgResourceHandle LodMapHandle = GImport(graph, pLodMap.get());

        RHI::RenderPass& buildLodMapPass = graph.AddRenderPass("BuildLodMapPass");

        buildLodMapPass.Read(terrainConsBufferHandle);
        buildLodMapPass.Read(NodeDescriptorsHandle, true);
        buildLodMapPass.Write(LodMapHandle, true);

        buildLodMapPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBuf(NodeDescriptorsHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(LodMapHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->FlushResourceBarriers();

            pContext->SetRootSignature(pBuildLodMapSignature.get());
            pContext->SetPipelineState(pBuildLodMapPSO.get());

            struct RootIndexBuffer
            {
                uint32_t consBufferIndex;
                uint32_t nodeDescriptorsBufferIndex;
                uint32_t lodMapIndex;
            };

            RootIndexBuffer rootIndexBuffer = RootIndexBuffer{ RegGetBufDefCBVIdx(terrainConsBufferHandle),
                                                               RegGetBufDefSRVIdx(NodeDescriptorsHandle),
                                                               RegGetTexDefUAVIdx(LodMapHandle) };

            pContext->SetConstantArray(0, sizeof(rootIndexBuffer) / sizeof(uint32_t), &rootIndexBuffer);

            pContext->Dispatch2D(160, 160, 8, 8);
            });

        //------------------------------------------------------------------------------------

        //------------------------------------------------------------------------------------

        RHI::RgResourceHandle buildPatchArgsHandle = graph.Create<RHI::D3D12Buffer>(buildPatchArgsBufferDesc);

        RHI::RgResourceHandle culledPatchListHandle = GImport(graph, CulledPatchListBuffer.get());

        RHI::RenderPass& buildPatchesPass = graph.AddRenderPass("BuildPatchesPass");

        buildPatchesPass.Read(terrainConsBufferHandle);
        buildPatchesPass.Read(terrainMinHeightHandle);
        buildPatchesPass.Read(terrainMaxHeightHandle);
        buildPatchesPass.Read(LodMapHandle);
        buildPatchesPass.Read(FinalNodeListHandle);
        buildPatchesPass.Write(culledPatchListHandle, true);

        buildPatchesPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBufCounter(FinalNodeListHandle), D3D12_RESOURCE_STATE_COPY_SOURCE);
            pContext->TransitionBarrier(RegGetBuf(buildPatchArgsHandle), D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->CopyBufferRegion(RegGetBuf(buildPatchArgsHandle), 0, RegGetBufCounter(FinalNodeListHandle), 0, sizeof(uint32_t));
            pContext->FillBuffer(RegGetBuf(buildPatchArgsHandle), sizeof(uint32_t), 1, sizeof(uint32_t) * 2);

            pContext->TransitionBarrier(RegGetBufCounter(culledPatchListHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->ResetCounter(RegGetBufCounter(culledPatchListHandle));
            pContext->FlushResourceBarriers();

            pContext->TransitionBarrier(RegGetBuf(culledPatchListHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBufCounter(culledPatchListHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBuf(buildPatchArgsHandle), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
            pContext->FlushResourceBarriers();

            pContext->SetRootSignature(pBuildPatchesSignature.get());
            pContext->SetPipelineState(pBuildPatchesPSO.get());

            struct RootIndexBuffer
            {
                uint32_t consBufferIndex;
                uint32_t minHeightmapIndex;
                uint32_t maxHeightmapIndex;
                uint32_t lodMapIndex;
                uint32_t finalNodeListBufferIndex;
                uint32_t culledPatchListBufferIndex;
            };

            RootIndexBuffer rootIndexBuffer = RootIndexBuffer{ RegGetBufDefCBVIdx(terrainConsBufferHandle),
                                                               RegGetTexDefSRVIdx(terrainMinHeightHandle),
                                                               RegGetTexDefSRVIdx(terrainMaxHeightHandle),
                                                               RegGetTexDefSRVIdx(LodMapHandle),
                                                               RegGetBufDefUAVIdx(FinalNodeListHandle),
                                                               RegGetBufDefUAVIdx(culledPatchListHandle) };

            pContext->SetConstantArray(0, sizeof(rootIndexBuffer) / sizeof(uint32_t), &rootIndexBuffer);

            pContext->DispatchIndirect(RegGetBuf(buildPatchArgsHandle), 0);
            });

        //------------------------------------------------------------------------------------



        /*
        bool needClearRenderTarget = initializeRenderTarget(graph, &passOutput);

        InternalTerrainRenderer& internalTerrainRenderer = m_render_scene->m_terrain_renderers[0].internalTerrainRenderer;
        std::shared_ptr<RHI::D3D12Texture> terrainHeightmap = internalTerrainRenderer.ref_terrain.terrain_heightmap;
        std::shared_ptr<RHI::D3D12Texture> terrainNormalmap = internalTerrainRenderer.ref_terrain.terrain_normalmap;

        std::shared_ptr<RHI::D3D12Buffer> uploadTransformBuffer = internalTerrainRenderer.ref_terrain.terrain_clipmap.instance_buffer.clip_transform_buffer;
        std::shared_ptr<RHI::D3D12Buffer> uploadMeshCountBuffer = internalTerrainRenderer.ref_terrain.terrain_clipmap.instance_buffer.clip_mesh_count_buffer;
        std::shared_ptr<RHI::D3D12Buffer> uploadBaseMeshCommandSigBuffer = internalTerrainRenderer.ref_terrain.terrain_clipmap.instance_buffer.clip_base_mesh_command_buffer;

        RHI::RgResourceHandle terrainHeightmapHandle = GImport(graph, terrainHeightmap.get());
        RHI::RgResourceHandle terrainNormalmapHandle = GImport(graph, terrainNormalmap.get());

        RHI::RgResourceHandle terrainMinHeightMapHandle = GImport(graph, terrainMinHeightMap.get());
        RHI::RgResourceHandle terrainMaxHeightMapHandle = GImport(graph, terrainMaxHeightMap.get());

        RHI::RgResourceHandle uploadBaseMeshCommandSigHandle = GImport(graph, uploadBaseMeshCommandSigBuffer.get());
        RHI::RgResourceHandle uploadClipmapTransformHandle   = GImport(graph, uploadTransformBuffer.get());
        RHI::RgResourceHandle uploadMeshCountHandle          = GImport(graph, uploadMeshCountBuffer.get());

        RHI::RgResourceHandle clipmapBaseCommandSighandle = GImport(graph, clipmapBaseCommandSigBuffer.get());
        RHI::RgResourceHandle clipmapTransformHandle      = GImport(graph, clipmapTransformBuffer.get());
        RHI::RgResourceHandle clipmapMeshCountHandle      = GImport(graph, clipmapMeshCountBuffer.get());

        RHI::RgResourceHandle camVisableClipmapHandle = GImport(graph, camVisableClipmapBuffer.get());

        std::vector<RHI::RgResourceHandle> dirVisableClipmapHandles {};
        int dirVisableClipmapBufferCount = dirVisableClipmapBuffers.m_VisableClipmapBuffers.size();
        for (int i = 0; i < dirVisableClipmapBufferCount; i++)
        {
            dirVisableClipmapHandles.push_back(GImport(graph, dirVisableClipmapBuffers.m_VisableClipmapBuffers[i].get()));
        }


        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;

        //=================================================================================
        //
        //=================================================================================
        if (!isTerrainMinMaxHeightReady)
        {
            isTerrainMinMaxHeightReady = true;

            RHI::RenderPass& genHeightMapMipMapPass = graph.AddRenderPass("GenerateHeightMapMipMapPass");

            genHeightMapMipMapPass.Read(terrainHeightmapHandle, true);
            genHeightMapMipMapPass.Read(terrainMinHeightMapHandle, true);
            genHeightMapMipMapPass.Read(terrainMaxHeightMapHandle, true);
            genHeightMapMipMapPass.Write(terrainMinHeightMapHandle, true);
            genHeightMapMipMapPass.Write(terrainMaxHeightMapHandle, true);

            genHeightMapMipMapPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

                {
                    pContext->TransitionBarrier(RegGetTex(terrainHeightmapHandle), D3D12_RESOURCE_STATE_COPY_SOURCE);
                    pContext->TransitionBarrier(RegGetTex(terrainMinHeightMapHandle), D3D12_RESOURCE_STATE_COPY_DEST, 0);
                    pContext->TransitionBarrier(RegGetTex(terrainMaxHeightMapHandle), D3D12_RESOURCE_STATE_COPY_DEST, 0);
                    pContext->FlushResourceBarriers();

                    const CD3DX12_TEXTURE_COPY_LOCATION src(terrainHeightmap->GetResource(), 0);

                    const CD3DX12_TEXTURE_COPY_LOCATION dst(terrainMinHeightMap->GetResource(), 0);
                    context->GetGraphicsCommandList()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

                    const CD3DX12_TEXTURE_COPY_LOCATION dst_max(terrainMaxHeightMap->GetResource(), 0);
                    context->GetGraphicsCommandList()->CopyTextureRegion(&dst_max, 0, 0, 0, &src, nullptr);

                    context->FlushResourceBarriers();
                }

                //--------------------------------------------------
                // ����MinHeightMap
                //--------------------------------------------------
                {
                    RHI::D3D12Texture* _SrcTexture = RegGetTex(terrainMinHeightMapHandle);
                    generateMipmapForTerrainHeightmap(pContext, _SrcTexture, true);
                }

                //--------------------------------------------------
                // ����MaxHeightMap
                //--------------------------------------------------
                {
                    RHI::D3D12Texture* _SrcTexture = RegGetTex(terrainMaxHeightMapHandle);
                    generateMipmapForTerrainHeightmap(pContext, _SrcTexture, false);
                }

            });
        }
        //=================================================================================
        // �����׼��CommandSigBuffer����
        //=================================================================================
        RHI::RenderPass& terrainCommandSigPreparePass = graph.AddRenderPass("TerrainClipmapPreparePass");

        terrainCommandSigPreparePass.Read(uploadBaseMeshCommandSigHandle, true);
        terrainCommandSigPreparePass.Read(uploadClipmapTransformHandle, true);
        terrainCommandSigPreparePass.Read(uploadMeshCountHandle, true);
        terrainCommandSigPreparePass.Write(clipmapBaseCommandSighandle, true);
        terrainCommandSigPreparePass.Write(clipmapTransformHandle, true);
        terrainCommandSigPreparePass.Write(clipmapMeshCountHandle, true);

        terrainCommandSigPreparePass.Execute(
            [=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

                pContext->TransitionBarrier(RegGetBuf(clipmapBaseCommandSighandle), D3D12_RESOURCE_STATE_COPY_DEST);
                pContext->TransitionBarrier(RegGetBuf(clipmapTransformHandle), D3D12_RESOURCE_STATE_COPY_DEST);
                pContext->TransitionBarrier(RegGetBuf(clipmapMeshCountHandle), D3D12_RESOURCE_STATE_COPY_DEST);
                pContext->FlushResourceBarriers();

                pContext->CopyBuffer(RegGetBuf(clipmapBaseCommandSighandle), RegGetBuf(uploadBaseMeshCommandSigHandle));
                pContext->CopyBuffer(RegGetBuf(clipmapTransformHandle), RegGetBuf(uploadClipmapTransformHandle));
                pContext->CopyBuffer(RegGetBuf(clipmapMeshCountHandle), RegGetBuf(uploadMeshCountHandle));

                pContext->FlushResourceBarriers();
            });

        //=================================================================================
        // ���������clipmap�ڵ�
        //=================================================================================
        RHI::RenderPass& terrainMainCullingPass = graph.AddRenderPass("TerrainMainCullingPass");

        terrainMainCullingPass.Read(perframeBufferHandle, true);
        terrainMainCullingPass.Read(clipmapBaseCommandSighandle, true);
        terrainMainCullingPass.Read(clipmapTransformHandle, true);
        terrainMainCullingPass.Read(clipmapMeshCountHandle, true);
        terrainMainCullingPass.Read(terrainMinHeightMapHandle, true);
        terrainMainCullingPass.Read(terrainMaxHeightMapHandle, true);
        terrainMainCullingPass.Write(camVisableClipmapHandle, true);

        terrainMainCullingPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBufCounter(camVisableClipmapHandle), D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->FlushResourceBarriers();
            pContext->ResetCounter(RegGetBufCounter(camVisableClipmapHandle));

            pContext->TransitionBarrier(RegGetBuf(perframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            pContext->TransitionBarrier(RegGetBuf(clipmapMeshCountHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            pContext->TransitionBarrier(RegGetBuf(clipmapBaseCommandSighandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetBuf(clipmapTransformHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(terrainMinHeightMapHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(terrainMaxHeightMapHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetBuf(camVisableClipmapHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBufCounter(camVisableClipmapHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->FlushResourceBarriers();

            pContext->SetRootSignature(pMainCamVisClipmapSignature.get());
            pContext->SetPipelineState(pMainCamVisClipmapGenPSO.get());

            struct RootIndexBuffer
            {
                uint32_t perFrameBufferIndex;
                uint32_t clipmapMeshCountIndex;
                uint32_t transformBufferIndex;
                uint32_t clipmapBaseCommandSigIndex;
                uint32_t terrainMinHeightmapIndex;
                uint32_t terrainMaxHeightmapIndex;
                uint32_t clipmapVisableBufferIndex;
            };

            RootIndexBuffer rootIndexBuffer = RootIndexBuffer {RegGetBufDefCBVIdx(perframeBufferHandle),
                                                               RegGetBufDefCBVIdx(clipmapMeshCountHandle),
                                                               RegGetBufDefSRVIdx(clipmapTransformHandle),
                                                               RegGetBufDefSRVIdx(clipmapBaseCommandSighandle),
                                                               RegGetTexDefSRVIdx(terrainMinHeightMapHandle),
                                                               RegGetTexDefSRVIdx(terrainMaxHeightMapHandle),
                                                               RegGetBufDefUAVIdx(camVisableClipmapHandle)};

            pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

            pContext->Dispatch1D(256, 8);
        });

        //=================================================================================
        // ��ԴShadow����
        //=================================================================================
        int dirShadowmapVisiableBufferCount = dirVisableClipmapBuffers.m_VisableClipmapBuffers.size();
        if (dirShadowmapVisiableBufferCount != 0)
        {
            RHI::RenderPass& dirLightShadowCullPass = graph.AddRenderPass("DirectionLightTerrainCullingPass");

            dirLightShadowCullPass.Read(perframeBufferHandle, true);
            dirLightShadowCullPass.Read(clipmapBaseCommandSighandle, true);
            dirLightShadowCullPass.Read(clipmapTransformHandle, true);
            dirLightShadowCullPass.Read(clipmapMeshCountHandle, true);
            dirLightShadowCullPass.Read(terrainMinHeightMapHandle, true);
            dirLightShadowCullPass.Read(terrainMaxHeightMapHandle, true);
            for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
            {
                dirLightShadowCullPass.Write(dirVisableClipmapHandles[i], true);
            }
            dirLightShadowCullPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->TransitionBarrier(RegGetBufCounter(dirVisableClipmapHandles[i]), D3D12_RESOURCE_STATE_COPY_DEST);
                }
                pContext->FlushResourceBarriers();
                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->ResetCounter(RegGetBufCounter(dirVisableClipmapHandles[i]));
                }
                pContext->TransitionBarrier(RegGetBuf(perframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                pContext->TransitionBarrier(RegGetBuf(clipmapMeshCountHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                pContext->TransitionBarrier(RegGetBuf(clipmapBaseCommandSighandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                pContext->TransitionBarrier(RegGetBuf(clipmapTransformHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                pContext->TransitionBarrier(RegGetTex(terrainMinHeightMapHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                pContext->TransitionBarrier(RegGetTex(terrainMaxHeightMapHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->TransitionBarrier(RegGetBuf(dirVisableClipmapHandles[i]), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    pContext->TransitionBarrier(RegGetBufCounter(dirVisableClipmapHandles[i]), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                }
                pContext->FlushResourceBarriers();

                for (uint32_t i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->SetRootSignature(pDirVisClipmapSignature.get());
                    pContext->SetPipelineState(pDirVisClipmapGenPSO.get());

                    struct RootIndexBuffer
                    {
                        uint32_t cascadeLevel;
                        uint32_t perFrameBufferIndex;
                        uint32_t clipmapMeshCountIndex;
                        uint32_t transformBufferIndex;
                        uint32_t clipmapBaseCommandSigIndex;
                        uint32_t terrainMinHeightmapIndex;
                        uint32_t terrainMaxHeightmapIndex;
                        uint32_t clipmapVisableBufferIndex;
                    };

                    RootIndexBuffer rootIndexBuffer = RootIndexBuffer {i,
                                                                       RegGetBufDefCBVIdx(perframeBufferHandle),
                                                                       RegGetBufDefCBVIdx(clipmapMeshCountHandle),
                                                                       RegGetBufDefSRVIdx(clipmapTransformHandle),
                                                                       RegGetBufDefSRVIdx(clipmapBaseCommandSighandle),
                                                                       RegGetTexDefSRVIdx(terrainMinHeightMapHandle),
                                                                       RegGetTexDefSRVIdx(terrainMaxHeightMapHandle),
                                                                       RegGetBufDefUAVIdx(dirVisableClipmapHandles[i])};

                    pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

                    pContext->Dispatch1D(256, 8);
                }
            });
        }



        /*
        RHI::RenderPass& terrainNodesBuildPass = graph.AddRenderPass("TerrainNodesBuildPass");

        terrainNodesBuildPass.Read(perframeBufferHandle, true); // RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        terrainNodesBuildPass.Read(terrainMinHeightMapHandle, true);
        terrainNodesBuildPass.Read(terrainMaxHeightMapHandle, true);
        terrainNodesBuildPass.Write(terrainPatchNodeHandle, true); // RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS

        terrainNodesBuildPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBufCounter(terrainPatchNodeHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->FlushResourceBarriers();
            pContext->ResetCounter(RegGetBufCounter(terrainPatchNodeHandle));
            pContext->TransitionBarrier(RegGetBufCounter(terrainPatchNodeHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->FlushResourceBarriers();

            pContext->TransitionBarrier(RegGetBuf(perframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            pContext->TransitionBarrier(RegGetBuf(terrainPatchNodeHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetTex(terrainMinHeightMapHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(terrainMaxHeightMapHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->FlushResourceBarriers();

            RHI::D3D12Texture* minHeightmap = registry->GetD3D12Texture(terrainMinHeightMapHandle);
            auto minHeightmapDesc = minHeightmap->GetDesc();

            RHI::D3D12ConstantBufferView* perframeCBVHandle = registry->GetD3D12Buffer(perframeBufferHandle)->GetDefaultCBV().get();
            RHI::D3D12UnorderedAccessView* terrainNodeHandleUAV = registry->GetD3D12Buffer(terrainPatchNodeHandle)->GetDefaultUAV().get();
            RHI::D3D12ShaderResourceView* terrainMinHeightmapSRV = registry->GetD3D12Texture(terrainMinHeightMapHandle)->GetDefaultSRV().get();
            RHI::D3D12ShaderResourceView* terrainMaxHeightmapSRV = registry->GetD3D12Texture(terrainMaxHeightMapHandle)->GetDefaultSRV().get();

            pContext->SetRootSignature(pIndirecTerrainPatchNodesGenSignature.get());
            pContext->SetPipelineState(pIndirecTerrainPatchNodesGenPSO.get());
            pContext->SetConstant(0, 0, perframeCBVHandle->GetIndex());
            pContext->SetConstant(0, 1, terrainNodeHandleUAV->GetIndex());
            pContext->SetConstant(0, 2, terrainMinHeightmapSRV->GetIndex());
            pContext->SetConstant(0, 3, terrainMaxHeightmapSRV->GetIndex());

            pContext->Dispatch2D(minHeightmapDesc.Width, minHeightmapDesc.Height, 8, 8);
        });

        //=================================================================================
        // ���������
        //=================================================================================

        RHI::RenderPass& terrainMainCullingPass = graph.AddRenderPass("TerrainMainCullingPass");

        terrainMainCullingPass.Read(perframeBufferHandle, true);
        terrainMainCullingPass.Read(terrainPatchNodeHandle, true);
        terrainMainCullingPass.Write(mainCamVisiableIndexHandle, true);

        terrainMainCullingPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBufCounter(mainCamVisiableIndexHandle), D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->FlushResourceBarriers();
            pContext->ResetCounter(RegGetBufCounter(mainCamVisiableIndexHandle));

            pContext->TransitionBarrier(RegGetBuf(perframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            pContext->TransitionBarrier(RegGetBuf(terrainPatchNodeHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetBufCounter(terrainPatchNodeHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetBuf(mainCamVisiableIndexHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBufCounter(mainCamVisiableIndexHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->FlushResourceBarriers();

            pContext->SetRootSignature(pPatchNodeVisToMainCamIndexGenSignature.get());
            pContext->SetPipelineState(pPatchNodeVisToMainCamIndexGenPSO.get());

            struct RootIndexBuffer
            {
                UINT meshPerFrameBufferIndex;
                UINT terrainPatchNodeIndex;
                UINT terrainPatchNodeCountIndex;
                UINT terrainVisNodeIdxIndex;
            };

            RootIndexBuffer rootIndexBuffer =
                RootIndexBuffer {RegGetBufDefCBVIdx(perframeBufferHandle),
                                 RegGetBufDefSRVIdx(terrainPatchNodeHandle),
                                 RegGetBufCounterSRVIdx(terrainPatchNodeHandle),
                                 RegGetBufDefUAVIdx(mainCamVisiableIndexHandle)};

            pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

            pContext->Dispatch1D(4096, 128);
        });

        //=================================================================================
        // ��ԴShadow����
        //=================================================================================
        int dirShadowmapVisiableBufferCount = dirShadowmapCommandBuffers.m_PatchNodeVisiableIndexBuffers.size();
        if (dirShadowmapVisiableBufferCount != 0)
        {
            RHI::RenderPass& dirLightShadowCullPass = graph.AddRenderPass("DirectionLightTerrainPatchNodeCullPass");

            dirLightShadowCullPass.Read(perframeBufferHandle, true);
            dirLightShadowCullPass.Read(terrainPatchNodeHandle, true);
            for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
            {
                dirLightShadowCullPass.Write(dirShadowPatchNodeVisiableIndexHandles[i], true);
            }
            dirLightShadowCullPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->TransitionBarrier(RegGetBufCounter(dirShadowPatchNodeVisiableIndexHandles[i]), D3D12_RESOURCE_STATE_COPY_DEST);
                }
                pContext->FlushResourceBarriers();
                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->ResetCounter(RegGetBufCounter(dirShadowPatchNodeVisiableIndexHandles[i]));
                }
                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->TransitionBarrier(RegGetBuf(dirShadowPatchNodeVisiableIndexHandles[i]), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    pContext->TransitionBarrier(RegGetBufCounter(dirShadowPatchNodeVisiableIndexHandles[i]), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                }
                pContext->TransitionBarrier(RegGetBuf(perframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                pContext->TransitionBarrier(RegGetBuf(terrainPatchNodeHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                pContext->TransitionBarrier(RegGetBufCounter(terrainPatchNodeHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                pContext->FlushResourceBarriers();

                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->SetRootSignature(pPatchNodeVisToDirCascadeIndexGenSignature.get());
                    pContext->SetPipelineState(pPatchNodeVisToDirCascadeIndexGenPSO.get());

                    struct RootIndexBuffer
                    {
                        UINT cascadeLevel;
                        UINT meshPerFrameBufferIndex;
                        UINT terrainPatchNodeIndex;
                        UINT terrainPatchNodeCountIndex;
                        UINT terrainVisNodeIdxIndex;
                    };

                    RootIndexBuffer rootIndexBuffer =
                        RootIndexBuffer {(UINT)i,
                                         RegGetBufDefCBVIdx(perframeBufferHandle),
                                         RegGetBufDefSRVIdx(terrainPatchNodeHandle),
                                         RegGetBufCounterSRVIdx(terrainPatchNodeHandle),
                                         RegGetBufDefUAVIdx(dirShadowPatchNodeVisiableIndexHandles[i])};

                    pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

                    pContext->Dispatch1D(4096, 128);
                }
            });
        }

        //=================================================================================
        // ��Ӱ�������CommandSignature׼��
        //=================================================================================

        if (dirShadowmapVisiableBufferCount != 0)
        {
            RHI::RenderPass& dirShadowCullingPass = graph.AddRenderPass("TerrainCommandSignatureForDirShadowPreparePass");

            dirShadowCullingPass.Read(mainCommandSigUploadBufferHandle, true);
            for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
            {
                dirShadowCullingPass.Read(dirShadowPatchNodeVisiableIndexHandles[i], true);
                dirShadowCullingPass.Write(dirShadowCommandSigBufferHandles[i], true);
            }

            dirShadowCullingPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

                pContext->TransitionBarrier(RegGetBuf(mainCommandSigUploadBufferHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ);
                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->TransitionBarrier(RegGetBufCounter(dirShadowPatchNodeVisiableIndexHandles[i]), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);
                    pContext->TransitionBarrier(RegGetBuf(dirShadowCommandSigBufferHandles[i]), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
                }
                pContext->FlushResourceBarriers();

                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->CopyBufferRegion(RegGetBuf(dirShadowCommandSigBufferHandles[i]),
                                               0,
                                               RegGetBuf(mainCommandSigUploadBufferHandle),
                                               0,
                                               sizeof(MoYu::TerrainCommandSignatureParams));
                    pContext->CopyBufferRegion(RegGetBuf(dirShadowCommandSigBufferHandles[i]),
                                               terrainInstanceCountOffset,
                                               RegGetBufCounter(dirShadowPatchNodeVisiableIndexHandles[i]),
                                               0,
                                               sizeof(int));
                    pContext->TransitionBarrier(RegGetBuf(dirShadowCommandSigBufferHandles[i]),
                                                D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
                }

                pContext->FlushResourceBarriers();

            });
        }

        //=================================================================================
        // ���Pass���
        //=================================================================================

        passOutput.terrainHeightmapHandle    = terrainHeightmapHandle;
        passOutput.terrainNormalmapHandle    = terrainNormalmapHandle;
        passOutput.maxHeightmapPyramidHandle = terrainMaxHeightMapHandle;
        passOutput.minHeightmapPyramidHandle = terrainMinHeightMapHandle;

        passOutput.transformBufferHandle     = clipmapTransformHandle;

        passOutput.mainCamVisCommandSigHandle = camVisableClipmapHandle;
        passOutput.dirVisCommandSigHandles.assign(dirVisableClipmapHandles.begin(), dirVisableClipmapHandles.end());
        */
    }

    void IndirectTerrainCullPass::destroy()
    {

    }

    bool IndirectTerrainCullPass::initializeRenderTarget(RHI::RenderGraph& graph, TerrainCullOutput* drawPassOutput)
    {
        return true;
    }

    void IndirectTerrainCullPass::generateMipmapForTerrainHeightmap(RHI::D3D12ComputeContext* context, RHI::D3D12Texture* _SrcTexture, bool genMin)
    {
        int numSubresource = _SrcTexture->GetNumSubresources();
        int iterCount = glm::ceil(numSubresource / 4.0f);
        for (int i = 0; i < iterCount; i++)
        {
            generateMipmapForTerrainHeightmap(context, _SrcTexture, 4 * i, genMin);
        }
    }

    void IndirectTerrainCullPass::generateMipmapForTerrainHeightmap(RHI::D3D12ComputeContext* pContext, RHI::D3D12Texture* _SrcTexture, int srcIndex, bool genMin)
    {
        int numSubresource = _SrcTexture->GetNumSubresources();
        int outMipCount = glm::max(0, glm::min(numSubresource - 1 - srcIndex, 4));

        pContext->TransitionBarrier(_SrcTexture, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, srcIndex);
        for (int i = 1; i < outMipCount; i++)
        {
            pContext->TransitionBarrier(_SrcTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, i + srcIndex);
        }
        pContext->FlushResourceBarriers();

        auto _SrcDesc = _SrcTexture->GetDesc();
        auto _Mip0SRVDesc = RHI::D3D12ShaderResourceView::GetDesc(_SrcTexture, false, srcIndex, 1);
        glm::uint _SrcIndex = _SrcTexture->CreateSRV(_Mip0SRVDesc)->GetIndex();

        int _width = _SrcDesc.Width >> (srcIndex + 1);
        int _height = _SrcDesc.Height >> (srcIndex + 1);

        glm::float2 _Mip1TexelSize = { 1.0f / _width, 1.0f / _height };
        glm::uint   _SrcMipLevel = srcIndex;
        glm::uint   _NumMipLevels = outMipCount;

        MipGenInBuffer maxMipGenBuffer = {};

        maxMipGenBuffer.SrcMipLevel = _SrcMipLevel;
        maxMipGenBuffer.NumMipLevels = _NumMipLevels;
        maxMipGenBuffer.TexelSize = _Mip1TexelSize;
        maxMipGenBuffer.SrcIndex = _SrcIndex;

        for (int i = 0; i < outMipCount; i++)
        {
            auto _Mip1UAVDesc = RHI::D3D12UnorderedAccessView::GetDesc(_SrcTexture, 0, srcIndex + i + 1);
            glm::uint _OutMip1Index = _SrcTexture->CreateUAV(_Mip1UAVDesc)->GetIndex();
            *(&maxMipGenBuffer.OutMip1Index + i) = _OutMip1Index;
        }

        pContext->SetRootSignature(RootSignatures::pGenerateMipsLinearSignature.get());
        if (genMin)
        {
            pContext->SetPipelineState(PipelineStates::pGenerateMinMipsLinearPSO.get());
        }
        else
        {
            pContext->SetPipelineState(PipelineStates::pGenerateMaxMipsLinearPSO.get());
        }

        pContext->SetConstantArray(0, sizeof(MipGenInBuffer) / sizeof(int), &maxMipGenBuffer);

        pContext->Dispatch2D(_width, _height, 8, 8);
    }

}
