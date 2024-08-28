#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    enum HistoryRejectionFlags
    {
        HRDepth = 0x1,
        HRReprojection = 0x2,
        HRPreviousDepth = 0x4,
        HRPosition = 0x8,
        HRNormal = 0x10,
        HRMotion = 0x20,
        HRCombined = HRDepth | HRReprojection | HRPreviousDepth | HRPosition | HRNormal | HRMotion,
        HRCombinedNoMotion = HRDepth | HRReprojection | HRPreviousDepth | HRPosition | HRNormal
    };

    class TemporalFilter
    {
    public:
        struct TemporalFilterInitInfo
        {
            RHI::D3D12Device* m_Device;
            RHI::RgTextureDesc m_ColorTexDesc;
            ShaderCompiler* m_ShaderCompiler;
            std::filesystem::path m_ShaderRootPath;
        };

        struct HistoryValidityPassData
        {
            // Other parameters
            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle cameraMotionVectorHandle;
            RHI::RgResourceHandle depthTextureHandle;
            RHI::RgResourceHandle historyDepthTextureHandle;
            RHI::RgResourceHandle normalBufferHandle;
            RHI::RgResourceHandle historyNormalTextureHandle;
            RHI::RgResourceHandle validationBufferHandle;
        };

        struct TemporalFilterPassData
        {
            RHI::RgResourceHandle perFrameBufferHandle;
            RHI::RgResourceHandle validationBufferHandle; // Validation buffer that tells us if the history should be ignored for a given pixel.
            RHI::RgResourceHandle cameraMotionVectorHandle; // Velocity buffer for history rejection
            RHI::RgResourceHandle depthTextureHandle; // Depth buffer of the current frame
            RHI::RgResourceHandle historyBufferHandle; // This buffer holds the previously accumulated signal
            RHI::RgResourceHandle denoiseInputTextureHandle; // Noisy Input Buffer from the current frame
            RHI::RgResourceHandle accumulationOutputTextureRWHandle; // Generic output buffer for our kernels
        };

        struct TemporalFilterStruct
        {
            // This holds the fov angle of a pixel
            float _PixelSpreadAngleTangent;
            // Value that tells us if the current history should be discarded based on scene-level data
            float _HistoryValidity;
            // Controls if the motion of the receiver is a valid rejection codition.
            int _ReceiverMotionRejection;
            // Controls if the motion of the receiver is a valid rejection codition.
            int _OccluderMotionRejection;
            // Contains history buffer size in xy and the uv scale in zw
            glm::float4 _HistorySizeAndScale;
            // Contains resolution multiplier in x, inverse in y, unused on zw
            glm::float4 _DenoiserResolutionMultiplierVals;
            // Value that tells us if the current history should be discarded based on scene-level data
            int _EnableExposureControl;

            glm::int3 _Unused0;
        };

        struct ShaderSignaturePSO
        {
            Shader m_Kernal;
            std::shared_ptr<RHI::D3D12RootSignature> pKernalSignature;
            std::shared_ptr<RHI::D3D12PipelineState> pKernalPSO;
        };

        void Init(TemporalFilterInitInfo InitInfo);

        void PrepareUniforms(RenderScene* render_scene, RenderCamera* camera);

        void HistoryValidity(RHI::RenderGraph& graph, HistoryValidityPassData& passData);
        void Denoise(RHI::RenderGraph& graph, TemporalFilterPassData& passData);

        TemporalFilterStruct mTemporalFilterStructData;
        std::shared_ptr<RHI::D3D12Buffer> mTemporalFilterStructBuffer;
        RHI::RgResourceHandle mTemporalFilterStructHandle;

        ShaderSignaturePSO mValidateHistory;

        ShaderSignaturePSO mTemporalAccumulationSingle;
        ShaderSignaturePSO mTemporalAccumulationColor;
        ShaderSignaturePSO mCopyHistory;

        RHI::RgTextureDesc colorTexDesc;
        RHI::D3D12Device* m_Device;
    };

};
