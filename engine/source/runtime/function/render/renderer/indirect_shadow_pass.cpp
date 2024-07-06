#include "runtime/function/render/renderer/indirect_shadow_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"

namespace MoYu
{

    void IndirectShadowPass::initialize(const ShadowPassInitInfo& init_info)
    {
        //Vector2 shadowmap_size = m_visiable_nodes.p_directional_light->m_shadowmap_size;

        //shadowmapTexDesc = RHI::RgTextureDesc("ShadowmapTex")
        //                       .SetFormat(DXGI_FORMAT_D32_FLOAT)
        //                       .SetExtent(shadowmap_size.x, shadowmap_size.y)
        //                       .SetAllowDepthStencil()
        //                       .SetClearValue(RHI::RgClearValue(0.0f, 0xff));

    }


    void IndirectShadowPass::prepareShadowmaps(std::shared_ptr<RenderResource> render_resource)
    {
        
    }

    void IndirectShadowPass::update(RHI::RenderGraph& graph, ShadowInputParameters& passInput, ShadowOutputParameters& passOutput)
    {
        //ShadowInputParameters  drawPassInput  = passInput;
        //ShadowOutputParameters drawPassOutput = passOutput;

        RHI::RgResourceHandle renderDataPerDrawHandle = passInput.renderDataPerDrawHandle;
        RHI::RgResourceHandle propertiesPerMaterialHandle = passInput.propertiesPerMaterialHandle;
        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;

        std::vector<RHI::RgResourceHandle> dirIndirectSortBufferHandles(passInput.dirIndirectSortBufferHandles);
        std::vector<RHI::RgResourceHandle> spotsIndirectSortBufferHandles(passInput.spotsIndirectSortBufferHandles);

        RHI::RenderPass& shadowpass = graph.AddRenderPass("IndirectShadowPass");

        shadowpass.Read(perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        shadowpass.Read(renderDataPerDrawHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        shadowpass.Read(propertiesPerMaterialHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);

        for (size_t i = 0; i < dirIndirectSortBufferHandles.size(); i++)
        {
            shadowpass.Read(dirIndirectSortBufferHandles[i],
                            false,
                            RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT,
                            RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT);
        }
        for (size_t i = 0; i < spotsIndirectSortBufferHandles.size(); i++)
        {
            shadowpass.Read(spotsIndirectSortBufferHandles[i],
                            false,
                            RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT,
                            RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT);
        }

        RHI::RgResourceHandle directionalShadowmapHandle = RHI::_DefaultRgResourceHandle;
        if (m_DirectionalShadowmap.p_LightShadowmap != nullptr)
        {
            passOutput.directionalShadowmapHandle = graph.Import<RHI::D3D12Texture>(m_DirectionalShadowmap.p_LightShadowmap.get());
            shadowpass.Write(passOutput.directionalShadowmapHandle, false, RHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE);
            directionalShadowmapHandle = passOutput.directionalShadowmapHandle;
        }

        for (size_t i = 0; i < m_SpotShadowmaps.size(); i++)
        {
            RHI::RgResourceHandle spotShadowMapHandle =
                graph.Import<RHI::D3D12Texture>(m_SpotShadowmaps[i].p_LightShadowmap.get());
            shadowpass.Write(spotShadowMapHandle, false, RHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE);
            passOutput.spotShadowmapHandle.push_back(spotShadowMapHandle);
        }
        std::vector<RHI::RgResourceHandle> spotShadowmapHandles = passOutput.spotShadowmapHandle;

        shadowpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12GraphicsContext* graphicContext = context->GetGraphicsContext();

            #define RegGetBufCounter(h) registry->GetD3D12Buffer(h)->GetCounterBuffer().get()
            
            if (m_DirectionalShadowmap.m_identifier != UndefCommonIdentifier)
            {
                RHI::D3D12Texture* pShadowmapStencilTex = registry->GetD3D12Texture(directionalShadowmapHandle);

                RHI::D3D12DepthStencilView* shadowmapStencilView = pShadowmapStencilTex->GetDefaultDSV().get();

                graphicContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                graphicContext->ClearRenderTarget(nullptr, shadowmapStencilView);
                graphicContext->SetRenderTarget(nullptr, shadowmapStencilView);

                graphicContext->SetRootSignature(RootSignatures::pIndirectDrawDirectionShadowmap.get());
                graphicContext->SetPipelineState(PipelineStates::pIndirectDrawDirectionShadowmap.get());

                glm::float2 shadowmap_size = m_DirectionalShadowmap.m_shadowmap_size;

                graphicContext->SetConstantBuffer(1, registry->GetD3D12Buffer(perframeBufferHandle)->GetGpuVirtualAddress());
                graphicContext->SetBufferSRV(2, registry->GetD3D12Buffer(renderDataPerDrawHandle));
                graphicContext->SetBufferSRV(3, registry->GetD3D12Buffer(propertiesPerMaterialHandle));

                for (uint32_t i = 0; i < m_DirectionalShadowmap.m_casccade; i++)
                {
                     RHIViewport _viewport = {};

                    _viewport.MinDepth = 0.0f;
                    _viewport.MaxDepth = 1.0f;
                    _viewport.Width    = 0.5 * shadowmap_size.x;
                    _viewport.Height   = 0.5 * shadowmap_size.y;

                    _viewport.TopLeftX = 0.5 * shadowmap_size.x * ((i & 0x1) != 0 ? 1 : 0);
                    _viewport.TopLeftY = 0.5 * shadowmap_size.y * ((i & 0x2) != 0 ? 1 : 0);

                    //graphicContext->SetViewport(RHIViewport {0.0f, 0.0f, (float)shadowmap_size.x, (float)shadowmap_size.y, 0.0f, 1.0f});
                    //graphicContext->SetScissorRect(RHIRect {0, 0, (int)shadowmap_size.x, (int)shadowmap_size.y});
                    graphicContext->SetViewport(RHIViewport {_viewport.TopLeftX, _viewport.TopLeftY, (float)_viewport.Width, (float)_viewport.Height, _viewport.MinDepth, _viewport.MaxDepth});
                    graphicContext->SetScissorRect(RHIRect {0, 0, (int)shadowmap_size.x, (int)shadowmap_size.y});

                    graphicContext->SetConstant(0, 1, i);

                    auto pDirectionCommandBuffer = registry->GetD3D12Buffer(dirIndirectSortBufferHandles[i]);
                    graphicContext->ExecuteIndirect(CommandSignatures::pIndirectDrawDirectionShadowmap.get(),
                                                        pDirectionCommandBuffer,
                                                        0,
                                                        HLSL::MeshLimit,
                                                        pDirectionCommandBuffer->GetCounterBuffer().get(),
                                                        0);
                }
            }

            for (size_t i = 0; i < m_SpotShadowmaps.size(); i++)
            {
                RHI::D3D12Texture* pShadowmapDepthTex = registry->GetD3D12Texture(spotShadowmapHandles[i]);

                RHI::D3D12DepthStencilView* shadowmapStencilView = pShadowmapDepthTex->GetDefaultDSV().get();

                graphicContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                glm::float2  shadowmap_size = m_SpotShadowmaps[i].m_shadowmap_size;
                uint32_t spot_index     = m_SpotShadowmaps[i].m_spot_index;

                graphicContext->SetRootSignature(RootSignatures::pIndirectDrawSpotShadowmap.get());
                graphicContext->SetPipelineState(PipelineStates::pIndirectDrawSpotShadowmap.get());
                graphicContext->SetViewport(RHIViewport {0.0f, 0.0f, (float)shadowmap_size.x, (float)shadowmap_size.y, 0.0f, 1.0f});
                graphicContext->SetScissorRect(RHIRect {0, 0, (int)shadowmap_size.x, (int)shadowmap_size.y});

                graphicContext->SetConstant(0, 1, spot_index);
                graphicContext->SetConstantBuffer(1, registry->GetD3D12Buffer(perframeBufferHandle)->GetGpuVirtualAddress());
                graphicContext->SetBufferSRV(2, registry->GetD3D12Buffer(renderDataPerDrawHandle));
                graphicContext->SetBufferSRV(3, registry->GetD3D12Buffer(propertiesPerMaterialHandle));

                graphicContext->ClearRenderTarget(nullptr, shadowmapStencilView);
                graphicContext->SetRenderTarget(nullptr, shadowmapStencilView);

                auto pSpotCommandBuffer = registry->GetD3D12Buffer(spotsIndirectSortBufferHandles[i]);

                graphicContext->ExecuteIndirect(CommandSignatures::pIndirectDrawSpotShadowmap.get(),
                                                pSpotCommandBuffer,
                                                0,
                                                HLSL::MeshLimit,
                                                pSpotCommandBuffer->GetCounterBuffer().get(),
                                                0);
            }

        });
    }

    void IndirectShadowPass::destroy() {}

}



