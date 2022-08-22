#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Pilot
{
    class IndirectDrawPass : public RenderPass
	{
    public:
        struct DrawPassInitInfo : public RenderPassInitInfo
        {

        };

        struct DrawInputParameters : public PassInput
        {};

        struct DrawOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle       backBufColor;
            RHI::D3D12RenderTargetView* backBufRtv;
        };

    public:
        ~IndirectDrawPass() { destroy(); }

        void initialize(const RenderPassInitInfo& init_info) override final;
        void update(RHI::D3D12CommandContext& context,
                    RHI::RenderGraph&         graph,
                    PassInput&                passInput,
                    PassOutput&               passOutput) override final;
        void
        draw(RHI::D3D12CommandContext& context, RHI::D3D12RenderTargetView* pRTV, int backBufWidth, int backBufHeight);
        void destroy() override final;

    private:
        std::shared_ptr<RHI::D3D12DynamicDescriptor<D3D12_SHADER_RESOURCE_VIEW_DESC>> pD3D12SRVDescriptor;

	};
}

