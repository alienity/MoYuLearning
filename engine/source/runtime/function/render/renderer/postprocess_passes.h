#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/renderer/msaa_resolve_pass.h"
#include "runtime/function/render/renderer/fxaa_pass.h"
#include "runtime/function/render/renderer/taa_pass.h"
#include "runtime/function/render/renderer/bloom_pass.h"
#include "runtime/function/render/renderer/extractLuma_pass.h"
#include "runtime/function/render/renderer/hdr_tonemapping_pass.h"
#include "runtime/function/render/renderer/exposure_pass.h"

namespace MoYu
{
    class PostprocessPasses : public RenderPass
	{
    public:
        struct PostprocessInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc colorTexDesc;
            ShaderCompiler*    m_ShaderCompiler;
        };

        struct PostprocessInputParameters : public PassInput
        {
            PostprocessInputParameters()
            {
                perframeBufferHandle.Invalidate();
                motionVectorHandle.Invalidate();
                inputSceneColorHandle.Invalidate();
                inputSceneDepthHandle.Invalidate();
            }

            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle motionVectorHandle;
            RHI::RgResourceHandle inputSceneColorHandle;
            RHI::RgResourceHandle inputSceneDepthHandle;
        };

        struct PostprocessOutputParameters : public PassOutput
        {
            PostprocessOutputParameters()
            {
                //postTargetColorHandle0.Invalidate();
                //postTargetColorHandle1.Invalidate();

                outputColorHandle.Invalidate();
            }

            //RHI::RgResourceHandle postTargetColorHandle0;
            //RHI::RgResourceHandle postTargetColorHandle1;

            RHI::RgResourceHandle outputColorHandle;
        };

    public:
        ~PostprocessPasses() { destroy(); }

        void initialize(const PostprocessInitInfo& init_info);
        void PreparePassData(std::shared_ptr<RenderResource> render_resource);
        void update(RHI::RenderGraph& graph, PostprocessInputParameters& passInput, PostprocessOutputParameters& passOutput);
        void destroy() override final;

    protected:
        //bool initializeResolveTarget(RHI::RenderGraph& graph, PostprocessOutputParameters* drawPassOutput);
        void Blit(RHI::RenderGraph& graph, RHI::RgResourceHandle& inputHandle, RHI::RgResourceHandle& outputHandle);

    private:
        RHI::RgTextureDesc colorTexDesc;

        std::shared_ptr<RHI::D3D12Texture> m_LumaBuffer;
        std::shared_ptr<RHI::D3D12Texture> m_TemporalMinBound;
        std::shared_ptr<RHI::D3D12Texture> m_TemporalMaxBound;

        std::shared_ptr<RHI::D3D12Buffer> m_ExposureBuffer;

        std::shared_ptr<MSAAResolvePass>    mResolvePass;
        std::shared_ptr<FXAAPass>           mFXAAPass;
        std::shared_ptr<TAAPass>            mTAAPass;
        std::shared_ptr<BloomPass>          mBloomPass;
        std::shared_ptr<ExtractLumaPass>    mExtractLumaPass;
        std::shared_ptr<HDRToneMappingPass> mHDRToneMappingPass;
        std::shared_ptr<ExposurePass>       mExposurePass;
	};
}

