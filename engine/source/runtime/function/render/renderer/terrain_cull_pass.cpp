#include "runtime/function/render/renderer/terrain_cull_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/core/math/moyu_math2.h"
#include "runtime/function/render/rhi/d3d12/d3d12_graphicsCommon.h"

#include "fmt/core.h"
#include <cassert>

namespace MoYu
{
#define RegGetBuf(h) registry->GetD3D12Buffer(h)
#define RegGetBufCounter(h) registry->GetD3D12Buffer(h)->GetCounterBuffer().get()
#define RegGetTex(h) registry->GetD3D12Texture(h)
#define RegGetBufDefCBVIdx(h) registry->GetD3D12Buffer(h)->GetDefaultCBV()->GetIndex()
#define RegGetBufDefSRVIdx(h) registry->GetD3D12Buffer(h)->GetDefaultSRV()->GetIndex()
#define RegGetBufDefUAVIdx(h) registry->GetD3D12Buffer(h)->GetDefaultUAV()->GetIndex()

    void IndirectTerrainCullPass::initialize(const TerrainCullInitInfo& init_info)
    {
        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        indirecTerrainPatchNodesGenCS =
            m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                            m_ShaderRootPath / "hlsl/IndirecTerrainPatchNodesGen.hlsl",
                                            ShaderCompileOptions(L"CSMain"));

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(4)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC,
                                             D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                             8)
                    .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_COMPARISON_ANISOTROPIC,
                                             D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                             8,
                                             D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL,
                                             D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pIndirecTerrainPatchNodesGenSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }

        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pIndirecTerrainPatchNodesGenSignature.get());
            psoStream.CS            = &indirecTerrainPatchNodesGenCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pIndirecTerrainPatchNodesGenPSO =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"IndirecTerrainPatchNodesGenPSO", psoDesc);
        }

        //-----------------------------------------------------------

        terrainPatchNodeIndexBuffer = RHI::D3D12Buffer::Create(
            m_Device->GetLinkedDevice(),
            RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter,
            MaxTerrainNodeCount,
            sizeof(HLSL::TerrainPatchNode),
            L"TerrainPatchIndexBuffer");



    }

    void IndirectTerrainCullPass::prepareMeshData(std::shared_ptr<RenderResource> render_resource)
    {
        if (terrainMinHeightMap == nullptr || terrainMaxHeightMap == nullptr)
        {
            InternalTerrainRenderer& internalTerrainRenderer = m_render_scene->m_terrain_renderers[0].internalTerrainRenderer;

            std::shared_ptr<RHI::D3D12Texture> terrainHeightmap = internalTerrainRenderer.ref_terrain.terrain_heightmap;
            auto heightmapDesc = terrainHeightmap->GetDesc();

            terrainMinHeightMap =
                RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                                            heightmapDesc.Width,
                                            heightmapDesc.Height,
                                            8,
                                            heightmapDesc.Format,
                                            RHI::RHISurfaceCreateRandomWrite | RHI::RHISurfaceCreateMipmap,
                                            1,
                                            L"TerrainMinHeightmap",
                                            D3D12_RESOURCE_STATE_COMMON,
                                            std::nullopt);


            terrainMaxHeightMap =
                RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                                            heightmapDesc.Width,
                                            heightmapDesc.Height,
                                            8,
                                            heightmapDesc.Format,
                                            RHI::RHISurfaceCreateRandomWrite | RHI::RHISurfaceCreateMipmap,
                                            1,
                                            L"TerrainMaxHeightmap",
                                            D3D12_RESOURCE_STATE_COMMON,
                                            std::nullopt);
        }
    }

    void IndirectTerrainCullPass::update(RHI::RenderGraph& graph, TerrainCullInput& passInput, TerrainCullOutput& passOutput)
    {
        bool needClearRenderTarget = initializeRenderTarget(graph, &passOutput);

        InternalTerrainRenderer& internalTerrainRenderer = m_render_scene->m_terrain_renderers[0].internalTerrainRenderer;
        std::shared_ptr<RHI::D3D12Texture> terrainHeightmap = internalTerrainRenderer.ref_terrain.terrain_heightmap;
        std::shared_ptr<RHI::D3D12Texture> terrainNormalmap = internalTerrainRenderer.ref_terrain.terrain_normalmap;

        RHI::RgResourceHandle terrainHeightmapHandle = GImport(graph, terrainHeightmap.get());
        RHI::RgResourceHandle terrainNormalmapHandle = GImport(graph, terrainNormalmap.get());

        RHI::RgResourceHandle terrainMinHeightMapHandle = GImport(graph, terrainMinHeightMap.get());
        RHI::RgResourceHandle terrainMaxHeightMapHandle = GImport(graph, terrainMaxHeightMap.get());

        RHI::RgResourceHandle terrainPatchNodeHandle = GImport(graph, terrainPatchNodeIndexBuffer.get());

        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;

        //=================================================================================
        // 生成HeightMap的MaxHeightMap和MinHeightMap
        //=================================================================================
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
            // 生成MinHeightMap
            //--------------------------------------------------
            {
                RHI::D3D12Texture* _SrcTexture = RegGetTex(terrainMinHeightMapHandle);
                generateMipmapForTerrainHeightmap(pContext, _SrcTexture, true);
            }
            
            //--------------------------------------------------
            // 生成MaxHeightMap
            //--------------------------------------------------
            {
                RHI::D3D12Texture* _SrcTexture = RegGetTex(terrainMaxHeightMapHandle);
                generateMipmapForTerrainHeightmap(pContext, _SrcTexture, false);
            }
            
        });


        //=================================================================================
        // 根据相机位置生成Node节点
        //=================================================================================

        RHI::RenderPass& terrainNodesBuildPass = graph.AddRenderPass("TerrainNodesBuildPass");

        terrainNodesBuildPass.Read(passInput.perframeBufferHandle, true); // RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        terrainNodesBuildPass.Read(terrainMinHeightMapHandle, true);
        terrainNodesBuildPass.Read(terrainMaxHeightMapHandle, true);
        terrainNodesBuildPass.Write(terrainPatchNodeHandle, true); // RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS
        
        terrainNodesBuildPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

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
        int iterCount = numSubresource / 4;
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

        int _width  = _SrcDesc.Width >> (srcIndex + 1);
        int _height = _SrcDesc.Height >> (srcIndex + 1);

        glm::float2 _Mip1TexelSize = {1.0f / _width, 1.0f / _height};
        glm::uint   _SrcMipLevel   = srcIndex;
        glm::uint   _NumMipLevels  = outMipCount;

        MipGenInBuffer maxMipGenBuffer = {};

        maxMipGenBuffer.SrcMipLevel   = _SrcMipLevel;
        maxMipGenBuffer.NumMipLevels  = _NumMipLevels;
        maxMipGenBuffer.TexelSize     = _Mip1TexelSize;
        maxMipGenBuffer.SrcIndex      = _SrcIndex;

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
