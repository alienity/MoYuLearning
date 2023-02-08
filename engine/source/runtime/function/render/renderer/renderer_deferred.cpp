#include "renderer_deferred.h"

#include "runtime/function/render/renderer/renderer_registry.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Pilot
{
    DeferredRenderer::DeferredRenderer(RendererInitParams& renderInitParams) : Renderer(renderInitParams) {}

    void DeferredRenderer::Initialize()
    {
        RHI::D3D12Texture* pBackBufferResource = pSwapChain->GetCurrentBackBufferResource();
        
        CD3DX12_RESOURCE_DESC backDesc = pBackBufferResource->GetDesc();

        backBufferWidth   = backDesc.Width;
        backBufferHeight  = backDesc.Height;
        backBufferFormat  = backDesc.Format;
        depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

        pipleineColorFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
        pipleineDepthFormat = D3D12RHIUtils::GetBaseFormat(DXGI_FORMAT_D32_FLOAT);

        SetViewPort(0, 0, backBufferWidth, backBufferHeight);

        std::shared_ptr<ConfigManager> config_manager = g_runtime_global_context.m_config_manager;
        const std::filesystem::path&   shaderRootPath = config_manager->getShaderFolder();

        // initialize global objects
        Shaders::Compile(pCompiler, shaderRootPath);
        RootSignatures::Compile(pDevice);
        CommandSignatures::Compile(pDevice);
        PipelineStates::Compile(pipleineColorFormat, pipleineDepthFormat, backBufferFormat, depthBufferFormat, pDevice);

        InitGlobalBuffer();
        InitPass();
    }

    void DeferredRenderer::InitGlobalBuffer()
    {
        const FLOAT         renderTargetClearColor[4] = {0, 0, 0, 0};
        CD3DX12_CLEAR_VALUE renderTargetClearValue    = CD3DX12_CLEAR_VALUE(backBufferFormat, renderTargetClearColor);

        p_RenderTargetTex = RHI::D3D12Texture::Create2D(pDevice->GetLinkedDevice(),
                                                        viewport.width,
                                                        viewport.height,
                                                        1,
                                                        backBufferFormat,
                                                        RHI::RHISurfaceCreateRenderTarget,
                                                        1,
                                                        L"RenderTargetTexture",
                                                        D3D12_RESOURCE_STATE_COMMON,
                                                        renderTargetClearValue);
    }

    void DeferredRenderer::InitPass()
    {
        RenderPassCommonInfo renderPassCommonInfo = {
            &renderGraphAllocator, &renderGraphRegistry, pDevice, pWindowSystem};

        int sampleCount = EngineConfig::g_AntialiasingMode == EngineConfig::MSAA ? 4 : 1;

        // Prepare common resources
        RHI::RgTextureDesc colorTexDesc = RHI::RgTextureDesc("ColorBuffer")
                                              .SetFormat(pipleineColorFormat)
                                              .SetExtent(viewport.width, viewport.height)
                                              .SetAllowRenderTarget()
                                              .SetSampleCount(sampleCount)
                                              .SetClearValue(RHI::RgClearValue(0, 0, 0, 0));
        RHI::RgTextureDesc depthTexDesc = RHI::RgTextureDesc("DepthBuffer")
                                              .SetFormat(pipleineDepthFormat)
                                              .SetExtent(viewport.width, viewport.height)
                                              .SetAllowDepthStencil()
                                              .SetSampleCount(sampleCount)
                                              .SetClearValue(RHI::RgClearValue(0.0f, 0xff));

        // Cull pass
        {
            mIndirectCullPass = std::make_shared<IndirectCullPass>();
            mIndirectCullPass->setCommonInfo(renderPassCommonInfo);
            mIndirectCullPass->initialize({});
        }
        // Opaque drawing pass
        {
            IndirectDrawPass::DrawPassInitInfo drawPassInit;
            drawPassInit.colorTexDesc = colorTexDesc;
            drawPassInit.depthTexDesc = depthTexDesc;

            mIndirectOpaqueDrawPass = std::make_shared<IndirectDrawPass>();
            mIndirectOpaqueDrawPass->setCommonInfo(renderPassCommonInfo);
            mIndirectOpaqueDrawPass->initialize(drawPassInit);
        }
        // Skybox pass
        {
            SkyBoxPass::SkyBoxInitInfo drawPassInit;
            drawPassInit.colorTexDesc = colorTexDesc;
            drawPassInit.depthTexDesc = depthTexDesc;

            mSkyBoxPass = std::make_shared<SkyBoxPass>();
            mSkyBoxPass->setCommonInfo(renderPassCommonInfo);
            mSkyBoxPass->initialize(drawPassInit);
        }
        // Transparent drawing pass
        {
            IndirectDrawTransparentPass::DrawPassInitInfo drawPassInit;
            drawPassInit.colorTexDesc = colorTexDesc;
            drawPassInit.depthTexDesc = depthTexDesc;

            mIndirectTransparentDrawPass = std::make_shared<IndirectDrawTransparentPass>();
            mIndirectTransparentDrawPass->setCommonInfo(renderPassCommonInfo);
            mIndirectTransparentDrawPass->initialize(drawPassInit);
        }
        // resolve pass
        {
            RHI::RgTextureDesc resolveColorTexDesc = colorTexDesc;
            resolveColorTexDesc.SetSampleCount(1);
            RHI::RgTextureDesc resolveDepthTexDesc = depthTexDesc;
            resolveDepthTexDesc.SetSampleCount(1);

            MSAAResolvePass::MSAAResolveInitInfo resolvePassInit;
            resolvePassInit.colorTexDesc = resolveColorTexDesc;
            resolvePassInit.depthTexDesc = resolveDepthTexDesc;

            mResolvePass = std::make_shared<MSAAResolvePass>();
            mResolvePass->setCommonInfo(renderPassCommonInfo);
            mResolvePass->initialize(resolvePassInit);
        }
        // shadow pass
        {
            IndirectShadowPass::ShadowPassInitInfo shadowPassInit;

            mIndirectShadowPass = std::make_shared<IndirectShadowPass>();
            mIndirectShadowPass->setCommonInfo(renderPassCommonInfo);
            mIndirectShadowPass->initialize(shadowPassInit);
        }
        // UI drawing pass
        {
            mDisplayPass = std::make_shared<DisplayPass>();
        }
    }

    void DeferredRenderer::InitializeUIRenderBackend(WindowUI* window_ui)
    {
        RenderPassCommonInfo renderPassCommonInfo = {
            &renderGraphAllocator, &renderGraphRegistry, pDevice, pWindowSystem};

        mUIPass = std::make_shared<UIPass>();
        mUIPass->setCommonInfo(renderPassCommonInfo);
        UIPass::UIPassInitInfo uiPassInitInfo;
        uiPassInitInfo.window_ui = window_ui;
        mUIPass->initialize(uiPassInitInfo);

        window_ui->setGameView(p_RenderTargetTex->GetDefaultSRV()->GetGpuHandle(), backBufferWidth, backBufferHeight);
    }

    void DeferredRenderer::PreparePassData(std::shared_ptr<RenderResourceBase> render_resource)
    {
        mIndirectCullPass->prepareMeshData(render_resource);
        mIndirectShadowPass->prepareShadowmaps(render_resource);
        mSkyBoxPass->prepareMeshData(render_resource);
    }

    DeferredRenderer::~DeferredRenderer() 
    {
        mUIPass                      = nullptr;
        mIndirectCullPass            = nullptr;
        mIndirectShadowPass          = nullptr;
        mIndirectOpaqueDrawPass      = nullptr;
        mSkyBoxPass                  = nullptr;
        mIndirectTransparentDrawPass = nullptr;
        mResolvePass                 = nullptr;
        mDisplayPass                 = nullptr;

        // Release global objects
        RootSignatures::Release();
        CommandSignatures::Release();
        PipelineStates::Release();
    }

    void DeferredRenderer::OnRender(RHI::D3D12CommandContext* context)
    {
        IndirectCullPass::IndirectCullOutput indirectCullOutput;
        mIndirectCullPass->cullMeshs(context, &renderGraphRegistry, indirectCullOutput);

        RHI::D3D12Texture* pBackBufferResource = pSwapChain->GetCurrentBackBufferResource();

        RHI::RenderGraph graph(renderGraphAllocator, renderGraphRegistry);

        // backbuffer output
        RHI::RgResourceHandle backBufColorHandle    = graph.Import(pBackBufferResource);

        // game view output
        RHI::RgResourceHandle renderTargetColorHandle = graph.Import(p_RenderTargetTex.get());
        
        // indirect draw shadow
        IndirectShadowPass::ShadowInputParameters  mShadowmapIntputParams;
        IndirectShadowPass::ShadowOutputParameters mShadowmapOutputParams;

        mShadowmapIntputParams.pPerframeBuffer            = indirectCullOutput.pPerframeBuffer;
        mShadowmapIntputParams.pMeshBuffer                = indirectCullOutput.pMeshBuffer;
        mShadowmapIntputParams.pMaterialBuffer            = indirectCullOutput.pMaterialBuffer;
        mShadowmapIntputParams.p_DirectionalCommandBuffer = indirectCullOutput.p_DirShadowmapCommandBuffer;
        mShadowmapIntputParams.p_SpotCommandBuffer        = indirectCullOutput.p_SpotShadowmapCommandBuffers;
        mIndirectShadowPass->update(graph, mShadowmapIntputParams, mShadowmapOutputParams);


        // indirect opaque draw
        IndirectDrawPass::DrawInputParameters  mDrawIntputParams;
        IndirectDrawPass::DrawOutputParameters mDrawOutputParams;

        mDrawIntputParams.pPerframeBuffer               = indirectCullOutput.pPerframeBuffer;
        mDrawIntputParams.pMeshBuffer                   = indirectCullOutput.pMeshBuffer;
        mDrawIntputParams.pMaterialBuffer               = indirectCullOutput.pMaterialBuffer;
        mDrawIntputParams.pIndirectCommandBuffer        = indirectCullOutput.p_OpaqueDrawCommandBuffer;
        mDrawIntputParams.directionalShadowmapTexHandle = mShadowmapOutputParams.directionalShadowmapRGHandle.shadowmapTextureHandle;
        for (size_t i = 0; i < mShadowmapOutputParams.spotShadowmapRGHandle.size(); i++)
        {
            mDrawIntputParams.spotShadowmapTexHandles.push_back(mShadowmapOutputParams.spotShadowmapRGHandle[i].shadowmapTextureHandle);
        }
        mIndirectOpaqueDrawPass->update(graph, mDrawIntputParams, mDrawOutputParams);
        
        // skybox draw
        SkyBoxPass::DrawInputParameters  mSkyboxIntputParams;
        SkyBoxPass::DrawOutputParameters mSkyboxOutputParams;

        mSkyboxIntputParams.pPerframeBuffer         = indirectCullOutput.pPerframeBuffer;
        mSkyboxOutputParams.renderTargetColorHandle = mDrawOutputParams.renderTargetColorHandle;
        mSkyboxOutputParams.renderTargetDepthHandle = mDrawOutputParams.renderTargetDepthHandle;
        mSkyBoxPass->update(graph, mSkyboxIntputParams, mSkyboxOutputParams);

        // indirect transparent draw
        IndirectDrawTransparentPass::DrawInputParameters  mDrawTransIntputParams;
        IndirectDrawTransparentPass::DrawOutputParameters mDrawTransOutputParams;

        mDrawTransIntputParams.pPerframeBuffer   = indirectCullOutput.pPerframeBuffer;
        mDrawTransIntputParams.pMeshBuffer       = indirectCullOutput.pMeshBuffer;
        mDrawTransIntputParams.pMaterialBuffer   = indirectCullOutput.pMaterialBuffer;
        mDrawTransIntputParams.pIndirectCommandBuffer = indirectCullOutput.p_TransparentDrawCommandBuffer;
        mDrawTransIntputParams.directionalShadowmapTexHandle = mShadowmapOutputParams.directionalShadowmapRGHandle.shadowmapTextureHandle;
        for (size_t i = 0; i < mShadowmapOutputParams.spotShadowmapRGHandle.size(); i++)
        {
            mDrawTransIntputParams.spotShadowmapTexHandles.push_back(mShadowmapOutputParams.spotShadowmapRGHandle[i].shadowmapTextureHandle);
        }
        mDrawTransOutputParams.renderTargetColorHandle = mDrawOutputParams.renderTargetColorHandle;
        mDrawTransOutputParams.renderTargetDepthHandle = mDrawOutputParams.renderTargetDepthHandle;
        mIndirectTransparentDrawPass->update(graph, mDrawTransIntputParams, mDrawTransOutputParams);

        RHI::RgResourceHandle inputRTColorHandle = mDrawOutputParams.renderTargetColorHandle;

        // resolve rendertarget
        if (EngineConfig::g_AntialiasingMode == EngineConfig::MSAA)
        {
            MSAAResolvePass::DrawInputParameters  mMSAAResolveIntputParams;
            MSAAResolvePass::DrawOutputParameters mMSAAResolveOutputParams;

            mMSAAResolveIntputParams.renderTargetColorHandle = mDrawOutputParams.renderTargetColorHandle;
            mMSAAResolveIntputParams.renderTargetDepthHandle = mDrawOutputParams.renderTargetDepthHandle;

            mResolvePass->update(graph, mMSAAResolveIntputParams, mMSAAResolveOutputParams);

            inputRTColorHandle = mMSAAResolveOutputParams.resolveTargetColorHandle;
        }

        // display
        DisplayPass::DisplayInputParameters  mDisplayIntputParams;
        DisplayPass::DisplayOutputParameters mDisplayOutputParams;

        mDisplayIntputParams.inputRTColorHandle      = inputRTColorHandle;
        mDisplayOutputParams.renderTargetColorHandle = renderTargetColorHandle;

        mDisplayPass->update(graph, mDisplayIntputParams, mDisplayOutputParams);

        if (mUIPass != nullptr)
        {
            UIPass::UIInputParameters mUIIntputParams;
            UIPass::UIOutputParameters mUIOutputParams;

            mUIIntputParams.renderTargetColorHandle = mDisplayOutputParams.renderTargetColorHandle;

            mUIOutputParams.backBufColorHandle = backBufColorHandle;
            
            mUIPass->update(graph, mUIIntputParams, mUIOutputParams);
        }

        graph.Execute(context);

        {
            // Transfer the state of the backbuffer to Present
            context->TransitionBarrier(pBackBufferResource,
                                      D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT,
                                      D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                                      true);
        }


    }

}

