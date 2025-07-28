#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/renderer/pass_helper.h"

namespace MoYu
{
    // https://therealmjp.github.io/posts/bindless-texturing-for-deferred-rendering-and-decals/
    class IndirectTerrainBoundsDebugPass : public RenderPass
	{
    public:
        struct DrawPassInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc colorTexDesc;
            RHI::RgTextureDesc depthTexDesc;

            ShaderCompiler*       m_ShaderCompiler;
            std::filesystem::path m_ShaderRootPath;
        };

        struct DrawInputParameters : public PassInput
        {
            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle terrainConstantBufferHandle;
            RHI::RgResourceHandle terrainRenderDataHandle;
            RHI::RgResourceHandle culledPatchListBufferHandle;
            RHI::RgResourceHandle mainCamVisCmdSigHandle;
        };

        struct DrawOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle colorHandle;
            RHI::RgResourceHandle depthHandle;
        };
        
    public:
        ~IndirectTerrainBoundsDebugPass() { destroy(); }

        void prepareMatBuffer(std::shared_ptr<RenderResource> render_resource);

        void initialize(const DrawPassInitInfo& init_info);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

    private:
        Shader terrainBoundsDebugVS;
        Shader terrainBoundsDebugPS;
        std::shared_ptr<RHI::D3D12RootSignature> pTerrainBoundDebugSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pTerrainBoundDebugPSO;
        std::shared_ptr<RHI::D3D12CommandSignature> pTerrainBoundDebugCommandSignature;

        RHI::RgTextureDesc colorBufferDesc; // float4
        RHI::RgTextureDesc depthDesc;   // float
	};
}

