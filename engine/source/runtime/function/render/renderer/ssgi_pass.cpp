#include "runtime/function/render/renderer/ssgi_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/function/render/jitter_helper.h"
#include "runtime/function/render/render_helper.h"
#include "runtime/function/render/renderer/pass_helper.h"

#include <cassert>

namespace MoYu
{

	void SSGIPass::initialize(const SSGIInitInfo& init_info)
	{
        passIndex = 0;

        colorTexDesc = init_info.m_ColorTexDesc;

        indirectDiffuseHitPointDesc = RHI::RgTextureDesc("IndirectDiffuseHitPointTexture")
            .SetType(RHI::RgTextureType::Texture2D)
            .SetFormat(DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT)
            .SetExtent(colorTexDesc.Width, colorTexDesc.Height)
            .SetAllowUnorderedAccess(true);

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        {
            SSTraceGICS = m_ShaderCompiler->CompileShader(
                RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Lighting/ScreenSpaceLighting/ScreenSpaceTraceGICS.hlsl",
                ShaderCompileOptions(L"TRACE_GLOBAL_ILLUMINATION"));

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(16)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 1)
                    .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 1)
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pSSTraceGISignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pSSTraceGISignature.get());
            psoStream.CS                    = &SSTraceGICS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pSSTraceGIPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"ScreenSpaceTraceGIPSO", psoDesc);
            
        }

        {
            SSReporjectGICS = m_ShaderCompiler->CompileShader(
                RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Lighting/ScreenSpaceLighting/ScreenSpaceReprojectGICS.hlsl",
                ShaderCompileOptions(L"REPROJECT_GLOBAL_ILLUMINATION"));

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                .Add32BitConstants<0, 0>(16)
                .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 1)
                .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 1)
                .AllowResourceDescriptorHeapIndexing()
                .AllowSampleDescriptorHeapIndexing();

            pSSReporjectGISignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pSSReporjectGISignature.get());
            psoStream.CS                    = &SSReporjectGICS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pSSReporjectGIPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"ScreenSpaceReprojectGIPSO", psoDesc);
        }
    }

    void SSGIPass::prepareMetaData(std::shared_ptr<RenderResource> render_resource)
    {
        m_owenScrambled256Tex = m_render_scene->m_bluenoise_map.m_owenScrambled256Tex;
        m_scramblingTile8SPP = m_render_scene->m_bluenoise_map.m_scramblingTile8SPP;
        m_rankingTile8SPP = m_render_scene->m_bluenoise_map.m_rankingTile8SPP;
        m_scramblingTex = m_render_scene->m_bluenoise_map.m_scramblingTex;

        //===================================================================================

        if (pIndirectDiffuseTexture == nullptr)
        {
            pIndirectDiffuseTexture =
                RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                    colorTexDesc.Width,
                    colorTexDesc.Height,
                    8,
                    DXGI_FORMAT_R32G32B32A32_FLOAT,
                    RHI::RHISurfaceCreateRandomWrite,
                    1,
                    L"SSIndirectDiffuseTexture",
                    D3D12_RESOURCE_STATE_COMMON,
                    std::nullopt);
        }

        //===================================================================================

        if (pSSGIConsBuffer == nullptr)
        {
            pSSGIConsBuffer = RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                RHI::RHIBufferTargetNone,
                1,
                MoYu::AlignUp(sizeof(ScreenSpaceGIStruct), 256),
                L"SSGICB",
                RHI::RHIBufferModeDynamic,
                D3D12_RESOURCE_STATE_GENERIC_READ);
        }

        int frameIndex = m_Device->GetLinkedDevice()->m_FrameIndex;

        float n = RenderPass::m_render_camera->m_nearClipPlane;
        float f = RenderPass::m_render_camera->m_farClipPlane;
        float thicknessScale = 1.0f / (1.0f + 0.01f);
        float thicknessBias = -n / (f - n) * (0.01f * thicknessScale);

        mSSGIStruct._RayMarchingSteps = EngineConfig::g_SSGIConfig.SSGIRaySteps;
        mSSGIStruct._RayMarchingThicknessScale = thicknessScale;
        mSSGIStruct._RayMarchingThicknessBias = thicknessBias;
        mSSGIStruct._RayMarchingReflectsSky = 0;

        mSSGIStruct._RayMarchingFallbackHierarchy = 0;
        mSSGIStruct._IndirectDiffuseProbeFallbackFlag = 0;
        mSSGIStruct._IndirectDiffuseProbeFallbackBias = 0;
        mSSGIStruct._SsrStencilBit = 0;

        mSSGIStruct._IndirectDiffuseFrameIndex = frameIndex % 16;
        mSSGIStruct._ObjectMotionStencilBit = (int)EngineConfig::StencilUsage::Clear;
        mSSGIStruct._RayMarchingLowResPercentageInv = 1.0f;

        mSSGIStruct._ColorPyramidUvScaleAndLimitPrevFrame = glm::float4(1, 1, colorTexDesc.Width, colorTexDesc.Height);

        memcpy(pSSGIConsBuffer->GetCpuVirtualAddress<ScreenSpaceGIStruct>(), &mSSGIStruct, sizeof(mSSGIStruct));
    }

    void SSGIPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        
        RHI::RgResourceHandle mSSGIConsBufferHandle = graph.Import<RHI::D3D12Buffer>(pSSGIConsBuffer.get());
        RHI::RgResourceHandle mIndirectDiffuseHandle = graph.Import<RHI::D3D12Texture>(pIndirectDiffuseTexture.get());

        RHI::RgResourceHandle owenScrambledTextureHandle = graph.Import<RHI::D3D12Texture>(m_owenScrambled256Tex.get());
        RHI::RgResourceHandle scramblingTileXSPPHandle = graph.Import<RHI::D3D12Texture>(m_scramblingTile8SPP.get());
        RHI::RgResourceHandle rankingTileXSPPHandle = graph.Import<RHI::D3D12Texture>(m_rankingTile8SPP.get());
        
        RHI::RgResourceHandle indirectDiffuseHitPointTexHandle = graph.Create<RHI::D3D12Texture>(indirectDiffuseHitPointDesc);

        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle colorPyramidHandle = passInput.colorPyramidHandle;
        RHI::RgResourceHandle depthPyramidHandle = passInput.depthPyramidHandle;
        RHI::RgResourceHandle lastDepthPyramidHandle = passInput.lastDepthPyramidHandle;
        RHI::RgResourceHandle normalBufferHandle = passInput.normalBufferHandle;
        RHI::RgResourceHandle cameraMotionVectorHandle = passInput.cameraMotionVectorHandle;
        
        RHI::RenderPass& ssTraceGIPass = graph.AddRenderPass("SSTraceGIPass");

        ssTraceGIPass.Read(perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        ssTraceGIPass.Read(depthPyramidHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ssTraceGIPass.Read(normalBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ssTraceGIPass.Read(mSSGIConsBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        ssTraceGIPass.Read(owenScrambledTextureHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ssTraceGIPass.Read(scramblingTileXSPPHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ssTraceGIPass.Read(rankingTileXSPPHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ssTraceGIPass.Write(indirectDiffuseHitPointTexHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

        ssTraceGIPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->SetRootSignature(pSSTraceGISignature.get());
            pContext->SetPipelineState(pSSTraceGIPSO.get());

            struct RootIndexBuffer
            {
                uint32_t frameUniformIndex;
                uint32_t depthPyramidTextureIndex;
                uint32_t normalBufferIndex;
                uint32_t screenSpaceTraceGIStructIndex;
                uint32_t indirectDiffuseHitPointTextureRWIndex;
                uint32_t _OwenScrambledTextureIndex;
                uint32_t _ScramblingTileXSPPIndex;
                uint32_t _RankingTileXSPPIndex;
            };

            auto CreateHandleIndexFunc = [&registry](RHI::RgResourceHandle InHandle) {
                RHI::D3D12Texture* TempTex = registry->GetD3D12Texture(InHandle);
                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
                srvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MostDetailedMip = 0;
                srvDesc.Texture2D.MipLevels = 1;
                srvDesc.Texture2D.PlaneSlice = 0;
                srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
                std::shared_ptr<RHI::D3D12ShaderResourceView> InTexSRV = TempTex->CreateSRV(srvDesc);
                UINT InTexSRVIndex = InTexSRV->GetIndex();
                return InTexSRVIndex;
            };

            RootIndexBuffer rootIndexBuffer = RootIndexBuffer{ RegGetBufDefCBVIdx(perframeBufferHandle),
                                                               RegGetTexDefSRVIdx(depthPyramidHandle),
                                                               RegGetTexDefSRVIdx(normalBufferHandle),
                                                               RegGetBufDefCBVIdx(mSSGIConsBufferHandle),
                                                               RegGetTexDefUAVIdx(indirectDiffuseHitPointTexHandle),
                                                               CreateHandleIndexFunc(owenScrambledTextureHandle),
                                                               CreateHandleIndexFunc(scramblingTileXSPPHandle),
                                                               CreateHandleIndexFunc(rankingTileXSPPHandle) };

            pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

            pContext->Dispatch2D(colorTexDesc.Width, colorTexDesc.Height, 8, 8);
        });
        

        RHI::RenderPass& ssReprojectGIPass = graph.AddRenderPass("SSReprojectGIPass");

        ssReprojectGIPass.Read(perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        ssReprojectGIPass.Read(colorPyramidHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ssReprojectGIPass.Read(depthPyramidHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ssReprojectGIPass.Read(mSSGIConsBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        ssReprojectGIPass.Read(indirectDiffuseHitPointTexHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ssReprojectGIPass.Read(cameraMotionVectorHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ssReprojectGIPass.Read(lastDepthPyramidHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ssReprojectGIPass.Write(mIndirectDiffuseHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

        ssReprojectGIPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->SetRootSignature(pSSReporjectGISignature.get());
            pContext->SetPipelineState(pSSReporjectGIPSO.get());

            struct RootIndexBuffer
            {
                uint32_t frameUniformIndex;
                uint32_t colorPyramidTextureIndex;
                uint32_t depthPyramidTextureIndex;
                uint32_t screenSpaceTraceGIStructIndex;
                uint32_t _IndirectDiffuseHitPointTextureIndex;
                uint32_t _CameraMotionVectorsTextureIndex;
                uint32_t _HistoryDepthTextureIndex;
                uint32_t _IndirectDiffuseTextureRWIndex;
            };

            RootIndexBuffer rootIndexBuffer = RootIndexBuffer{ RegGetBufDefCBVIdx(perframeBufferHandle),
                                                               RegGetTexDefSRVIdx(colorPyramidHandle),
                                                               RegGetTexDefSRVIdx(depthPyramidHandle),
                                                               RegGetBufDefCBVIdx(mSSGIConsBufferHandle),
                                                               RegGetTexDefSRVIdx(indirectDiffuseHitPointTexHandle),
                                                               RegGetTexDefSRVIdx(cameraMotionVectorHandle),
                                                               RegGetTexDefSRVIdx(lastDepthPyramidHandle),
                                                               RegGetTexDefUAVIdx(mIndirectDiffuseHandle) };

            pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

            pContext->Dispatch2D(colorTexDesc.Width, colorTexDesc.Height, 8, 8);
        });

        passOutput.ssrOutHandle = mIndirectDiffuseHandle;
    }

    void SSGIPass::destroy()
    {



    }

}
