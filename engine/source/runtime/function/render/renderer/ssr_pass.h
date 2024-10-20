#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    struct SSROutput
    {
        SSROutput()
        {
            motionVectorHandle.Invalidate();
            depthHandle.Invalidate();
        }

        RHI::RgResourceHandle motionVectorHandle;
        RHI::RgResourceHandle depthHandle;
    };

    class SSRPass : public RenderPass
	{
    public:
        struct SSRInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc    m_ColorTexDesc;
            ShaderCompiler*       m_ShaderCompiler;
            std::filesystem::path m_ShaderRootPath;
        };

        struct DrawInputParameters : public PassInput
        {
            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle worldNormalHandle;
            RHI::RgResourceHandle mrraMapHandle;
            RHI::RgResourceHandle maxDepthPtyramidHandle;
            RHI::RgResourceHandle lastFrameColorHandle;
        };

        struct DrawOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle ssrOutHandle;
        };

    public:
        ~SSRPass() { destroy(); }

        void initialize(const SSRInitInfo& init_info);
        void prepareMetaData(std::shared_ptr<RenderResource> render_resource);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

        std::shared_ptr<RHI::D3D12Texture> getCurTemporalResult();
        std::shared_ptr<RHI::D3D12Texture> getPrevTemporalResult();

    private:
        int passIndex;

        Shader SSRRaycastCS;
        std::shared_ptr<RHI::D3D12RootSignature> pSSRRaycastSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pSSRRaycastPSO;

        Shader SSRResolveCS;
        std::shared_ptr<RHI::D3D12RootSignature> pSSRResolveSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pSSRResolvePSO;

        Shader SSRTemporalCS;
        std::shared_ptr<RHI::D3D12RootSignature> pSSRTemporalSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pSSRTemporalPSO;

        RHI::RgTextureDesc colorTexDesc;
        RHI::RgTextureDesc raycastResultDesc;
        RHI::RgTextureDesc raycastMaskDesc;
        RHI::RgTextureDesc resolveResultDesc;

        std::shared_ptr<RHI::D3D12Texture> m_bluenoise;
        std::shared_ptr<RHI::D3D12Texture> p_temporalResults[2];
	};
}

