#include "runtime/function/render/renderer/skybox_pass.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraph.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace MoYu
{

	void SkyBoxPass::initialize(const SkyBoxInitInfo& init_info)
	{
        colorTexDesc = init_info.colorTexDesc;
        depthTexDesc = init_info.depthTexDesc;
	}

    void SkyBoxPass::prepareMeshData(std::shared_ptr<RenderResource> render_resource)
    {
        std::shared_ptr<RHI::D3D12Texture> pSpecularTex = m_render_scene->m_skybox_map.m_skybox_specular_map;

        specularIBLTexIndex = pSpecularTex->GetDefaultSRV()->GetIndex();
        specularIBLTexLevel = 0.0f;
    }

    void SkyBoxPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {

        bool needClearRenderTarget = initializeRenderTarget(graph, &passOutput);

        //DrawInputParameters*  drawPassInput  = &passInput;
        //DrawOutputParameters* drawPassOutput = &passOutput;

        RHI::RenderPass& drawpass = graph.AddRenderPass("SkyboxPass");

        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle renderTargetColorHandle = passOutput.renderTargetColorHandle;
        RHI::RgResourceHandle renderTargetDepthHandle = passOutput.renderTargetDepthHandle;

        drawpass.Read(passInput.perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        drawpass.Read(passOutput.renderTargetColorHandle, true);
        drawpass.Read(passOutput.renderTargetDepthHandle, true);

        drawpass.Write(passOutput.renderTargetColorHandle, false, RHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
        drawpass.Write(passOutput.renderTargetDepthHandle, false, RHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE);

        drawpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12GraphicsContext* graphicContext = context->GetGraphicsContext();

            RHI::D3D12Texture* pRenderTargetColor = registry->GetD3D12Texture(renderTargetColorHandle);
            RHI::D3D12Texture* pRenderTargetDepth = registry->GetD3D12Texture(renderTargetDepthHandle);

            RHI::D3D12RenderTargetView* renderTargetView = pRenderTargetColor->GetDefaultRTV().get();
            RHI::D3D12DepthStencilView* depthStencilView = pRenderTargetDepth->GetDefaultDSV().get();

            graphicContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            graphicContext->SetViewport(RHIViewport {0.0f, 0.0f, (float)colorTexDesc.Width, (float)colorTexDesc.Height, 0.0f, 1.0f});
            graphicContext->SetScissorRect(RHIRect {0, 0, (int)colorTexDesc.Width, (int)colorTexDesc.Height});

            if (needClearRenderTarget)
            {
                graphicContext->ClearRenderTarget(renderTargetView, depthStencilView);
            }
            graphicContext->SetRenderTarget(renderTargetView, depthStencilView);

            graphicContext->SetRootSignature(RootSignatures::pSkyBoxRootSignature.get());
            graphicContext->SetPipelineState(PipelineStates::pSkyBoxPSO.get());
            graphicContext->SetConstants(0, specularIBLTexIndex, specularIBLTexLevel);
            //graphicContext->SetConstantBuffer(1, pPerframeBuffer->GetGpuVirtualAddress());
            graphicContext->SetConstantBuffer(1, registry->GetD3D12Buffer(perframeBufferHandle)->GetGpuVirtualAddress());
            
            graphicContext->Draw(3);
        });
    }


    void SkyBoxPass::destroy()
    {

    }

    bool SkyBoxPass::initializeRenderTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput)
    {
        bool needClearRenderTarget = false;
        if (!drawPassOutput->renderTargetColorHandle.IsValid())
        {
            needClearRenderTarget                   = true;
            drawPassOutput->renderTargetColorHandle = graph.Create<RHI::D3D12Texture>(colorTexDesc);
        }
        if (!drawPassOutput->renderTargetDepthHandle.IsValid())
        {
            needClearRenderTarget                   = true;
            drawPassOutput->renderTargetDepthHandle = graph.Create<RHI::D3D12Texture>(depthTexDesc);
        }
        return needClearRenderTarget;
    }

}
