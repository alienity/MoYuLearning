#pragma once

#include "runtime/function/render/rhi/d3d12/d3d12_rootSignature.h"
#include "runtime/function/render/rhi/rhi.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraphCommon.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraphRegistry.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraph.h"
#include "runtime/function/render/renderer/renderer_config.h"

// clang-format off
struct Shaders
{
    static constexpr LPCWSTR g_VSEntryPoint = L"VSMain";
    static constexpr LPCWSTR g_MSEntryPoint = L"MSMain";
    static constexpr LPCWSTR g_PSEntryPoint = L"PSMain";
    static constexpr LPCWSTR g_CSEntryPoint = L"CSMain";

	// Vertex Shaders
    struct VS
    {
        inline static Shader ScreenQuadPresentVS;
        inline static Shader IndirectDrawVS;
        inline static Shader IndirectDrawVS_BRDF;
        inline static Shader IndirectDrawDirectionShadowmapVS;
        inline static Shader IndirectDrawSpotShadowmapVS;
        inline static Shader SkyBoxVS;
    };

    // Mesh Shaders
    struct MS
    {
        //inline static Shader Meshlet;
    };

    // Pixel Shaders
    struct PS
    {
        inline static Shader PresentSDRPS;
        inline static Shader IndirectDrawPS;
        inline static Shader IndirectDrawPS_BRDF;
        inline static Shader IndirectDrawShadowmapPS;
        inline static Shader SkyBoxPS;
    };

	// Compute Shaders
    struct CS
    {
        inline static Shader BitonicIndirectArgsCS;
        inline static Shader Bitonic32PreSortCS;
        inline static Shader Bitonic32InnerSortCS;
        inline static Shader Bitonic32OuterSortCS;
        inline static Shader Bitonic64PreSortCS;
        inline static Shader Bitonic64InnerSortCS;
        inline static Shader Bitonic64OuterSortCS;
        inline static Shader IndirectCullForSort;
        inline static Shader IndirectCull;
        inline static Shader IndirectCullArgs;
        inline static Shader IndirectCullGrab;
        inline static Shader IndirectCullDirectionShadowmap;
        inline static Shader IndirectCullSpotShadowmap;

        inline static Shader GenerateMipsLinearCS;
        inline static Shader GenerateMipsLinearOddCS;
        inline static Shader GenerateMipsLinearOddXCS;
        inline static Shader GenerateMipsLinearOddYCS;
        inline static Shader GenerateMaxMipsLinearCS;
        inline static Shader GenerateMaxMipsLinearOddCS;
        inline static Shader GenerateMaxMipsLinearOddXCS;
        inline static Shader GenerateMaxMipsLinearOddYCS;
        inline static Shader GenerateMinMipsLinearCS;
        inline static Shader GenerateMinMipsLinearOddCS;
        inline static Shader GenerateMinMipsLinearOddXCS;
        inline static Shader GenerateMinMipsLinearOddYCS;
    };

    static void Compile(ShaderCompiler* Compiler, const std::filesystem::path& ShaderPath)
    {
        // VS
        {
            ShaderCompileOptions Options(g_VSEntryPoint);
            VS::ScreenQuadPresentVS = Compiler->CompileShader(RHI_SHADER_TYPE::Vertex, ShaderPath / "hlsl/ScreenQuadPresentVS.hlsl", Options);
            VS::IndirectDrawVS = Compiler->CompileShader(RHI_SHADER_TYPE::Vertex, ShaderPath / "hlsl/IndirectDraw.hlsl", Options);

            VS::IndirectDrawVS_BRDF = Compiler->CompileShader(RHI_SHADER_TYPE::Vertex, ShaderPath / "hlsl/IndirectDrawPS.hlsl", Options);

            ShaderCompileOptions DrawDirectionOptions(g_VSEntryPoint);
            DrawDirectionOptions.SetDefine({L"DIRECTIONSHADOW"}, {L"1"});
            VS::IndirectDrawDirectionShadowmapVS = Compiler->CompileShader(RHI_SHADER_TYPE::Vertex, ShaderPath / "hlsl/IndirectDrawShadowmap.hlsl", DrawDirectionOptions);

            ShaderCompileOptions DrawSpotOptions(g_VSEntryPoint);
            DrawSpotOptions.SetDefine({L"SPOTSHADOW"}, {L"1"});
            VS::IndirectDrawSpotShadowmapVS = Compiler->CompileShader(RHI_SHADER_TYPE::Vertex, ShaderPath / "hlsl/IndirectDrawShadowmap.hlsl", DrawSpotOptions);

            ShaderCompileOptions DrawSkyBoxOptions(g_VSEntryPoint);
            VS::SkyBoxVS = Compiler->CompileShader(RHI_SHADER_TYPE::Vertex, ShaderPath / "hlsl/SkyBoxVS.hlsl", DrawSkyBoxOptions);
        }

		// PS
        {
            ShaderCompileOptions Options(g_PSEntryPoint);
            PS::PresentSDRPS = Compiler->CompileShader(RHI_SHADER_TYPE::Pixel, ShaderPath / "hlsl/PresentSDRPS.hlsl", Options);
            PS::IndirectDrawPS = Compiler->CompileShader(RHI_SHADER_TYPE::Pixel, ShaderPath / "hlsl/IndirectDraw.hlsl", Options);
            PS::IndirectDrawPS_BRDF = Compiler->CompileShader(RHI_SHADER_TYPE::Pixel, ShaderPath / "hlsl/IndirectDrawPS.hlsl", Options);
            PS::IndirectDrawShadowmapPS = Compiler->CompileShader(RHI_SHADER_TYPE::Pixel, ShaderPath / "hlsl/IndirectDrawShadowmap.hlsl", Options);
            PS::SkyBoxPS = Compiler->CompileShader(RHI_SHADER_TYPE::Pixel, ShaderPath / "hlsl/SkyBoxPS.hlsl", Options);
        }

		// CS
        {
            constexpr LPCWSTR g_CSSortEntryPoint = L"main";

            CS::BitonicIndirectArgsCS = Compiler->CompileShader(RHI_SHADER_TYPE::Compute, ShaderPath / "hlsl/BitonicIndirectArgsCS.hlsl", ShaderCompileOptions(g_CSSortEntryPoint));
            CS::Bitonic32PreSortCS    = Compiler->CompileShader(RHI_SHADER_TYPE::Compute, ShaderPath / "hlsl/Bitonic32PreSortCS.hlsl", ShaderCompileOptions(g_CSSortEntryPoint));
            CS::Bitonic32InnerSortCS  = Compiler->CompileShader(RHI_SHADER_TYPE::Compute, ShaderPath / "hlsl/Bitonic32InnerSortCS.hlsl", ShaderCompileOptions(g_CSSortEntryPoint));
            CS::Bitonic32OuterSortCS  = Compiler->CompileShader(RHI_SHADER_TYPE::Compute, ShaderPath / "hlsl/Bitonic32OuterSortCS.hlsl", ShaderCompileOptions(g_CSSortEntryPoint));
            CS::Bitonic64PreSortCS    = Compiler->CompileShader(RHI_SHADER_TYPE::Compute, ShaderPath / "hlsl/Bitonic64PreSortCS.hlsl", ShaderCompileOptions(g_CSSortEntryPoint));
            CS::Bitonic64InnerSortCS  = Compiler->CompileShader(RHI_SHADER_TYPE::Compute, ShaderPath / "hlsl/Bitonic64InnerSortCS.hlsl", ShaderCompileOptions(g_CSSortEntryPoint));
            CS::Bitonic64OuterSortCS  = Compiler->CompileShader(RHI_SHADER_TYPE::Compute, ShaderPath / "hlsl/Bitonic64OuterSortCS.hlsl", ShaderCompileOptions(g_CSSortEntryPoint));

            CS::IndirectCullForSort = Compiler->CompileShader(RHI_SHADER_TYPE::Compute, ShaderPath / "hlsl/IndirectCullForSort.hlsl", ShaderCompileOptions(g_CSEntryPoint));

            ShaderCompileOptions meshCSOption(g_CSEntryPoint);
            CS::IndirectCull = Compiler->CompileShader(RHI_SHADER_TYPE::Compute, ShaderPath / "hlsl/IndirectCull.hlsl", meshCSOption);
            CS::IndirectCullArgs = Compiler->CompileShader(RHI_SHADER_TYPE::Compute, ShaderPath / "hlsl/IndirectCullArgsCS.hlsl", meshCSOption);
            CS::IndirectCullGrab = Compiler->CompileShader(RHI_SHADER_TYPE::Compute, ShaderPath / "hlsl/IndirectCullGrabCS.hlsl", meshCSOption);

            ShaderCompileOptions directionCSOption(g_CSEntryPoint);
            directionCSOption.SetDefine({L"DIRECTIONSHADOW"}, {L"1"});
            CS::IndirectCullDirectionShadowmap = Compiler->CompileShader(RHI_SHADER_TYPE::Compute, ShaderPath / "hlsl/IndirectCullShadowmap.hlsl", directionCSOption);

            ShaderCompileOptions spotCSOption(g_CSEntryPoint);
            spotCSOption.SetDefine({L"SPOTSHADOW"}, {L"1"});
            CS::IndirectCullSpotShadowmap = Compiler->CompileShader(RHI_SHADER_TYPE::Compute, ShaderPath / "hlsl/IndirectCullShadowmap.hlsl", spotCSOption);

            {
            #define GetCompiledShader(ShaderName) Compiler->CompileShader(RHI_SHADER_TYPE::Compute, ShaderPath / ShaderName, ShaderCompileOptions(L"main"));

                CS::GenerateMipsLinearCS = GetCompiledShader("hlsl/GenerateMipsLinearCS.hlsl")
                CS::GenerateMipsLinearOddCS = GetCompiledShader("hlsl/GenerateMipsLinearOddCS.hlsl")
                CS::GenerateMipsLinearOddXCS = GetCompiledShader("hlsl/GenerateMipsLinearOddXCS.hlsl")
                CS::GenerateMipsLinearOddYCS = GetCompiledShader("hlsl/GenerateMipsLinearOddYCS.hlsl")

                CS::GenerateMaxMipsLinearCS = GetCompiledShader("hlsl/GenerateMaxMipsLinearCS.hlsl")
                CS::GenerateMaxMipsLinearOddCS = GetCompiledShader("hlsl/GenerateMaxMipsLinearOddCS.hlsl")
                CS::GenerateMaxMipsLinearOddXCS = GetCompiledShader("hlsl/GenerateMaxMipsLinearOddXCS.hlsl")
                CS::GenerateMaxMipsLinearOddYCS = GetCompiledShader("hlsl/GenerateMaxMipsLinearOddYCS.hlsl")

                CS::GenerateMinMipsLinearCS = GetCompiledShader("hlsl/GenerateMinMipsLinearCS.hlsl")
                CS::GenerateMinMipsLinearOddCS = GetCompiledShader("hlsl/GenerateMinMipsLinearOddCS.hlsl")
                CS::GenerateMinMipsLinearOddXCS = GetCompiledShader("hlsl/GenerateMinMipsLinearOddXCS.hlsl")
                CS::GenerateMinMipsLinearOddYCS = GetCompiledShader("hlsl/GenerateMinMipsLinearOddYCS.hlsl")
            }

        }
    }


};

struct Libraries
{
    inline static Library PathTrace;

    static void Compile(ShaderCompiler* Compiler)
    { 

    }
};

struct RootSignatures
{
    inline static std::shared_ptr<RHI::D3D12RootSignature> pBitonicSortRootSignature;

    inline static std::shared_ptr<RHI::D3D12RootSignature> pFullScreenPresent;
    inline static std::shared_ptr<RHI::D3D12RootSignature> pIndirectCullForSort;
    inline static std::shared_ptr<RHI::D3D12RootSignature> pIndirectCull;
    inline static std::shared_ptr<RHI::D3D12RootSignature> pIndirectCullArgs;
    inline static std::shared_ptr<RHI::D3D12RootSignature> pIndirectCullGrab;
    inline static std::shared_ptr<RHI::D3D12RootSignature> pIndirectCullDirectionShadowmap;
    inline static std::shared_ptr<RHI::D3D12RootSignature> pIndirectCullSpotShadowmap;
    inline static std::shared_ptr<RHI::D3D12RootSignature> pIndirectDraw;
    inline static std::shared_ptr<RHI::D3D12RootSignature> pIndirectDrawDirectionShadowmap;
    inline static std::shared_ptr<RHI::D3D12RootSignature> pIndirectDrawSpotShadowmap;
    inline static std::shared_ptr<RHI::D3D12RootSignature> pSkyBoxRootSignature;

    inline static std::shared_ptr<RHI::D3D12RootSignature> pGenerateMipsLinearSignature;

    static void Compile(RHI::D3D12Device* pDevice)
    {
        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(2)
                    .AddDescriptorTable(RHI::D3D12DescriptorTable(1).AddSRVRange<0, 0>(1, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0))
                    .AddDescriptorTable(RHI::D3D12DescriptorTable(1).AddUAVRange<0, 0>(1, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0))
                    .Add32BitConstants<1, 0>(2)
                    .AllowResourceDescriptorHeapIndexing();

            pBitonicSortRootSignature = std::make_shared<RHI::D3D12RootSignature>(pDevice, rootSigDesc);
        }

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(1)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pFullScreenPresent = std::make_shared<RHI::D3D12RootSignature>(pDevice, rootSigDesc);
        }

        {
            RHI::RootSignatureDesc rootSigDesc = RHI::RootSignatureDesc()
                                                     .Add32BitConstants<0, 0>(64)
                                                     .AllowResourceDescriptorHeapIndexing()
                                                     .AllowSampleDescriptorHeapIndexing();

            pIndirectCullForSort = std::make_shared<RHI::D3D12RootSignature>(pDevice, rootSigDesc);
        }
        {
            RHI::RootSignatureDesc rootSigDesc = RHI::RootSignatureDesc()
                                                     .AddConstantBufferView<0, 0>()
                                                     .AddShaderResourceView<0, 0>()
                                                     .AddShaderResourceView<1, 0>()
                                                     .AddUnorderedAccessViewWithCounter<0, 0>()
                                                     .AddUnorderedAccessViewWithCounter<1, 0>()
                                                     .AllowResourceDescriptorHeapIndexing()
                                                     .AllowSampleDescriptorHeapIndexing();

            pIndirectCull = std::make_shared<RHI::D3D12RootSignature>(pDevice, rootSigDesc);
        }

        {
            RHI::RootSignatureDesc rootSigDesc = RHI::RootSignatureDesc()
                                                     .AddShaderResourceView<0, 0>()
                                                     .AddUnorderedAccessView<0, 0>()
                                                     .AllowResourceDescriptorHeapIndexing()
                                                     .AllowSampleDescriptorHeapIndexing();

            pIndirectCullArgs = std::make_shared<RHI::D3D12RootSignature>(pDevice, rootSigDesc);
        }

        {
            RHI::RootSignatureDesc rootSigDesc = RHI::RootSignatureDesc()
                                                     .AddShaderResourceView<0, 0>()
                                                     .AddShaderResourceView<1, 0>()
                                                     .AddShaderResourceView<2, 0>()
                                                     .AddUnorderedAccessViewWithCounter<0, 0>()
                                                     .AllowResourceDescriptorHeapIndexing()
                                                     .AllowSampleDescriptorHeapIndexing();

            pIndirectCullGrab = std::make_shared<RHI::D3D12RootSignature>(pDevice, rootSigDesc);
        }

        {
            RHI::RootSignatureDesc rootSigDesc = RHI::RootSignatureDesc()
                                                     .Add32BitConstants<0,0>(1)
                                                     .AddConstantBufferView<1, 0>()
                                                     .AddShaderResourceView<0, 0>()
                                                     .AddShaderResourceView<1, 0>()
                                                     .AddUnorderedAccessViewWithCounter<0, 0>()
                                                     .AllowResourceDescriptorHeapIndexing()
                                                     .AllowSampleDescriptorHeapIndexing();

            pIndirectCullDirectionShadowmap = std::make_shared<RHI::D3D12RootSignature>(pDevice, rootSigDesc);
        }

        {
            RHI::RootSignatureDesc rootSigDesc = RHI::RootSignatureDesc()
                                                     .Add32BitConstants<0, 0>(1)
                                                     .AddConstantBufferView<1, 0>()
                                                     .AddShaderResourceView<0, 0>()
                                                     .AddShaderResourceView<1, 0>()
                                                     .AddUnorderedAccessViewWithCounter<0, 0>()
                                                     .AllowResourceDescriptorHeapIndexing()
                                                     .AllowSampleDescriptorHeapIndexing();

            pIndirectCullSpotShadowmap = std::make_shared<RHI::D3D12RootSignature>(pDevice, rootSigDesc);
        }

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(1)
                    .AddConstantBufferView<1, 0>()
                    .AddShaderResourceView<0, 0>()
                    .AddShaderResourceView<1, 0>()
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                    .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_COMPARISON_ANISOTROPIC,
                                       D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                       8,
                                       D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL,
                                       D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pIndirectDraw = std::make_shared<RHI::D3D12RootSignature>(pDevice, rootSigDesc);
        }

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(2)
                    .AddConstantBufferView<1, 0>()
                    .AddShaderResourceView<0, 0>()
                    .AddShaderResourceView<1, 0>()
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC,
                                       D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                       8)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pIndirectDrawDirectionShadowmap = std::make_shared<RHI::D3D12RootSignature>(pDevice, rootSigDesc);
        }

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(2)
                    .AddConstantBufferView<1, 0>()
                    .AddShaderResourceView<0, 0>()
                    .AddShaderResourceView<1, 0>()
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC,
                                       D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                       8)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pIndirectDrawSpotShadowmap = std::make_shared<RHI::D3D12RootSignature>(pDevice, rootSigDesc);
        }

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(2)
                    .AddConstantBufferView<1, 0>()
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pSkyBoxRootSignature = std::make_shared<RHI::D3D12RootSignature>(pDevice, rootSigDesc);
        }

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(12)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pGenerateMipsLinearSignature = std::make_shared<RHI::D3D12RootSignature>(pDevice, rootSigDesc);
        }
    }

    static void Release()
    {
        pBitonicSortRootSignature       = nullptr;
        pFullScreenPresent              = nullptr;
        pIndirectCullForSort            = nullptr;
        pIndirectCull                   = nullptr;
        pIndirectCullArgs               = nullptr;
        pIndirectCullGrab               = nullptr;
        pIndirectCullDirectionShadowmap = nullptr;
        pIndirectCullSpotShadowmap      = nullptr;
        pIndirectDraw                   = nullptr;
        pIndirectDrawDirectionShadowmap = nullptr;
        pIndirectDrawSpotShadowmap      = nullptr;
        pSkyBoxRootSignature            = nullptr;
        pGenerateMipsLinearSignature    = nullptr;
    }
};

struct CommandSignatures
{
    inline static std::shared_ptr<RHI::D3D12CommandSignature> pDispatchIndirectCommandSignature;

    inline static std::shared_ptr<RHI::D3D12CommandSignature> pIndirectDraw;
    inline static std::shared_ptr<RHI::D3D12CommandSignature> pIndirectDrawDirectionShadowmap;
    inline static std::shared_ptr<RHI::D3D12CommandSignature> pIndirectDrawSpotShadowmap;

    static void Compile(RHI::D3D12Device* pDevice)
    {
        {
            RHI::CommandSignatureDesc Builder(1);
            Builder.AddDispatch();

            pDispatchIndirectCommandSignature =
                std::make_shared<RHI::D3D12CommandSignature>(pDevice, Builder);
        }

        {
            RHI::CommandSignatureDesc Builder(4);
            Builder.AddConstant(0, 0, 1);
            Builder.AddVertexBufferView(0);
            Builder.AddIndexBufferView();
            Builder.AddDrawIndexed();

            pIndirectDraw =
                std::make_shared<RHI::D3D12CommandSignature>(pDevice, Builder, RootSignatures::pIndirectDraw->GetApiHandle());
        }
        {
            RHI::CommandSignatureDesc Builder(4);
            Builder.AddConstant(0, 0, 1);
            Builder.AddVertexBufferView(0);
            Builder.AddIndexBufferView();
            Builder.AddDrawIndexed();

            pIndirectDrawDirectionShadowmap = std::make_shared<RHI::D3D12CommandSignature>(
                pDevice, Builder, RootSignatures::pIndirectDrawDirectionShadowmap->GetApiHandle());
        }
        {
            RHI::CommandSignatureDesc Builder(4);
            Builder.AddConstant(0, 0, 1);
            Builder.AddVertexBufferView(0);
            Builder.AddIndexBufferView();
            Builder.AddDrawIndexed();

            pIndirectDrawSpotShadowmap = std::make_shared<RHI::D3D12CommandSignature>(
                pDevice, Builder, RootSignatures::pIndirectDrawSpotShadowmap->GetApiHandle());
        }
    }

    static void Release()
    {
        pDispatchIndirectCommandSignature = nullptr;
        pIndirectDraw                     = nullptr;
        pIndirectDrawDirectionShadowmap   = nullptr;
        pIndirectDrawSpotShadowmap        = nullptr;
    }
};

struct PipelineStates
{
    inline static std::shared_ptr<RHI::D3D12PipelineState> pBitonicIndirectArgsPSO;
    inline static std::shared_ptr<RHI::D3D12PipelineState> pBitonic32PreSortPSO;
    inline static std::shared_ptr<RHI::D3D12PipelineState> pBitonic32InnerSortPSO;
    inline static std::shared_ptr<RHI::D3D12PipelineState> pBitonic32OuterSortPSO;
    inline static std::shared_ptr<RHI::D3D12PipelineState> pBitonic64PreSortPSO;
    inline static std::shared_ptr<RHI::D3D12PipelineState> pBitonic64InnerSortPSO;
    inline static std::shared_ptr<RHI::D3D12PipelineState> pBitonic64OuterSortPSO;

    inline static std::shared_ptr<RHI::D3D12PipelineState> pFullScreenPresent;
    inline static std::shared_ptr<RHI::D3D12PipelineState> pIndirectCullForSort;
    inline static std::shared_ptr<RHI::D3D12PipelineState> pIndirectCull;
    inline static std::shared_ptr<RHI::D3D12PipelineState> pIndirectCullArgs;
    inline static std::shared_ptr<RHI::D3D12PipelineState> pIndirectCullGrab;
    inline static std::shared_ptr<RHI::D3D12PipelineState> pIndirectCullDirectionShadowmap;
    inline static std::shared_ptr<RHI::D3D12PipelineState> pIndirectCullSpotShadowmap;

    inline static std::shared_ptr<RHI::D3D12PipelineState> pIndirectDraw;
    inline static std::shared_ptr<RHI::D3D12PipelineState> pIndirectDraw_BRDF;
    inline static std::shared_ptr<RHI::D3D12PipelineState> pIndirectDrawTransparent;
    inline static std::shared_ptr<RHI::D3D12PipelineState> pIndirectDrawDirectionShadowmap;
    inline static std::shared_ptr<RHI::D3D12PipelineState> pIndirectDrawSpotShadowmap;

    inline static std::shared_ptr<RHI::D3D12PipelineState> pSkyBoxPSO;

    inline static std::shared_ptr<RHI::D3D12PipelineState> pGenerateMipsLinearPSO;
    inline static std::shared_ptr<RHI::D3D12PipelineState> pGenerateMipsLinearOddPSO;
    inline static std::shared_ptr<RHI::D3D12PipelineState> pGenerateMipsLinearOddXPSO;
    inline static std::shared_ptr<RHI::D3D12PipelineState> pGenerateMipsLinearOddYPSO;

    inline static std::shared_ptr<RHI::D3D12PipelineState> pGenerateMaxMipsLinearPSO;
    inline static std::shared_ptr<RHI::D3D12PipelineState> pGenerateMaxMipsLinearOddPSO;
    inline static std::shared_ptr<RHI::D3D12PipelineState> pGenerateMaxMipsLinearOddXPSO;
    inline static std::shared_ptr<RHI::D3D12PipelineState> pGenerateMaxMipsLinearOddYPSO;

    inline static std::shared_ptr<RHI::D3D12PipelineState> pGenerateMinMipsLinearPSO;
    inline static std::shared_ptr<RHI::D3D12PipelineState> pGenerateMinMipsLinearOddPSO;
    inline static std::shared_ptr<RHI::D3D12PipelineState> pGenerateMinMipsLinearOddXPSO;
    inline static std::shared_ptr<RHI::D3D12PipelineState> pGenerateMinMipsLinearOddYPSO;

    static std::shared_ptr<RHI::D3D12PipelineState> CreatePSO(RHI::D3D12RootSignature* rootSig, Shader& cs, RHI::D3D12Device* device, std::wstring name)
    {
        struct PsoStream
        {
            PipelineStateStreamRootSignature RootSignature;
            PipelineStateStreamCS            CS;
        } psoStream;
        psoStream.RootSignature         = PipelineStateStreamRootSignature(rootSig);
        psoStream.CS                    = &cs;
        PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

        return std::make_shared<RHI::D3D12PipelineState>(device, name, psoDesc);
    }

    static void Compile(DXGI_FORMAT PiplineRtFormat, DXGI_FORMAT PipelineDsFormat, DXGI_FORMAT RtFormat, DXGI_FORMAT DsFormat, RHI::D3D12Device* pDevice)
    {
        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(RootSignatures::pBitonicSortRootSignature.get());
            psoStream.CS            = &Shaders::CS::BitonicIndirectArgsCS;

            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pBitonicIndirectArgsPSO = std::make_shared<RHI::D3D12PipelineState>(pDevice, L"Bitonic Sort: Indirect Args CS", psoDesc);

            psoStream.CS = &Shaders::CS::Bitonic32PreSortCS;
            pBitonic32PreSortPSO = std::make_shared<RHI::D3D12PipelineState>(pDevice, L"Bitonic Sort: 32 Pre Sort CS", psoDesc);

            psoStream.CS = &Shaders::CS::Bitonic32InnerSortCS;
            pBitonic32InnerSortPSO = std::make_shared<RHI::D3D12PipelineState>(pDevice, L"Bitonic Sort: 32 Inner Sort CS", psoDesc);

            psoStream.CS = &Shaders::CS::Bitonic32OuterSortCS;
            pBitonic32OuterSortPSO = std::make_shared<RHI::D3D12PipelineState>(pDevice, L"Bitonic Sort: 32 Outer Sort CS", psoDesc);

            psoStream.CS = &Shaders::CS::Bitonic64PreSortCS;
            pBitonic64PreSortPSO = std::make_shared<RHI::D3D12PipelineState>(pDevice, L"Bitonic Sort: 64 Pre Sort CS", psoDesc);

            psoStream.CS = &Shaders::CS::Bitonic64InnerSortCS;
            pBitonic64InnerSortPSO = std::make_shared<RHI::D3D12PipelineState>(pDevice, L"Bitonic Sort: 64 Inner Sort CS", psoDesc);

            psoStream.CS = &Shaders::CS::Bitonic64OuterSortCS;
            pBitonic64OuterSortPSO = std::make_shared<RHI::D3D12PipelineState>(pDevice, L"Bitonic Sort: 64 Outer Sort CS", psoDesc);
        }
        {
            RHI::D3D12InputLayout InputLayout;

            RHIDepthStencilState DepthStencilState;
            DepthStencilState.DepthEnable = false;

            RHIRenderTargetState RenderTargetState;
            RenderTargetState.RTFormats[0]     = RtFormat; // DXGI_FORMAT_R32G32B32A32_FLOAT;
            RenderTargetState.NumRenderTargets = 1;
            RenderTargetState.DSFormat         = DsFormat; // DXGI_FORMAT_D32_FLOAT;

            struct PsoStream
            {
                PipelineStateStreamRootSignature     RootSignature;
                PipelineStateStreamInputLayout       InputLayout;
                PipelineStateStreamPrimitiveTopology PrimitiveTopologyType;
                PipelineStateStreamVS                VS;
                PipelineStateStreamPS                PS;
                PipelineStateStreamDepthStencilState DepthStencilState;
                PipelineStateStreamRenderTargetState RenderTargetState;
            } psoStream;
            psoStream.RootSignature      = PipelineStateStreamRootSignature(RootSignatures::pFullScreenPresent.get());
            psoStream.InputLayout        = &InputLayout;
            psoStream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
            psoStream.VS                    = &Shaders::VS::ScreenQuadPresentVS;
            psoStream.PS                    = &Shaders::PS::PresentSDRPS;
            psoStream.DepthStencilState     = DepthStencilState;
            psoStream.RenderTargetState     = RenderTargetState;

            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pFullScreenPresent = std::make_shared<RHI::D3D12PipelineState>(pDevice, L"FullScreenPresent", psoDesc);
        }
        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(RootSignatures::pIndirectCullForSort.get());
            psoStream.CS            = &Shaders::CS::IndirectCullForSort;

            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pIndirectCullForSort = std::make_shared<RHI::D3D12PipelineState>(pDevice, L"IndirectCullForSort", psoDesc);
        }
        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(RootSignatures::pIndirectCull.get());
            psoStream.CS            = &Shaders::CS::IndirectCull;

            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pIndirectCull = std::make_shared<RHI::D3D12PipelineState>(pDevice, L"IndirectCull", psoDesc);
        }
        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(RootSignatures::pIndirectCullArgs.get());
            psoStream.CS            = &Shaders::CS::IndirectCullArgs;

            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pIndirectCullArgs = std::make_shared<RHI::D3D12PipelineState>(pDevice, L"IndirectCullArgs", psoDesc);
        }
        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(RootSignatures::pIndirectCullGrab.get());
            psoStream.CS            = &Shaders::CS::IndirectCullGrab;

            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pIndirectCullGrab = std::make_shared<RHI::D3D12PipelineState>(pDevice, L"IndirectCullGrab", psoDesc);
        }
        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(RootSignatures::pIndirectCullDirectionShadowmap.get());
            psoStream.CS = &Shaders::CS::IndirectCullDirectionShadowmap;

            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pIndirectCullDirectionShadowmap = std::make_shared<RHI::D3D12PipelineState>(pDevice, L"IndirectCullDirectionShadowmap", psoDesc);
        }
        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(RootSignatures::pIndirectCullSpotShadowmap.get());
            psoStream.CS         = &Shaders::CS::IndirectCullSpotShadowmap;

            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pIndirectCullSpotShadowmap = std::make_shared<RHI::D3D12PipelineState>(pDevice, L"IndirectCullSpotShadowmap", psoDesc);
        }
        {
            RHI::D3D12InputLayout InputLayout = MoYu::D3D12MeshVertexPositionNormalTangentTexture::InputLayout;
            
            RHIDepthStencilState DepthStencilState;
            DepthStencilState.DepthEnable = true;
            DepthStencilState.DepthFunc   = RHI_COMPARISON_FUNC::GreaterEqual;

            RHIRenderTargetState RenderTargetState;
            RenderTargetState.RTFormats[0]     = PiplineRtFormat; // DXGI_FORMAT_R32G32B32A32_FLOAT;
            RenderTargetState.NumRenderTargets = 1;
            RenderTargetState.DSFormat         = PipelineDsFormat; // DXGI_FORMAT_D32_FLOAT;

            RHISampleState SampleState;
            SampleState.Count = EngineConfig::g_AntialiasingMode == EngineConfig::MSAA ? EngineConfig::g_MASSConfig.m_MSAASampleCount : 1;

            struct PsoStream
            {
                PipelineStateStreamRootSignature     RootSignature;
                PipelineStateStreamInputLayout       InputLayout;
                PipelineStateStreamPrimitiveTopology PrimitiveTopologyType;
                PipelineStateStreamRasterizerState   RasterrizerState;
                PipelineStateStreamVS                VS;
                PipelineStateStreamPS                PS;
                PipelineStateStreamDepthStencilState DepthStencilState;
                PipelineStateStreamRenderTargetState RenderTargetState;
                PipelineStateStreamSampleState       SampleState;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(RootSignatures::pIndirectDraw.get());
            psoStream.InputLayout           = &InputLayout;
            psoStream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
            psoStream.RasterrizerState      = RHIRasterizerState();
            psoStream.VS                    = &Shaders::VS::IndirectDrawVS;
            psoStream.PS                    = &Shaders::PS::IndirectDrawPS;
            psoStream.DepthStencilState     = DepthStencilState;
            psoStream.RenderTargetState     = RenderTargetState;
            psoStream.SampleState           = SampleState;

            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};
            pIndirectDraw = std::make_shared<RHI::D3D12PipelineState>(pDevice, L"IndirectDraw", psoDesc);
        }
        {
            RHI::D3D12InputLayout InputLayout = MoYu::D3D12MeshVertexPositionNormalTangentTexture::InputLayout;
            
            RHIDepthStencilState DepthStencilState;
            DepthStencilState.DepthEnable = true;
            DepthStencilState.DepthFunc   = RHI_COMPARISON_FUNC::GreaterEqual;

            RHIRenderTargetState RenderTargetState;
            RenderTargetState.RTFormats[0]     = PiplineRtFormat; // DXGI_FORMAT_R32G32B32A32_FLOAT;
            RenderTargetState.NumRenderTargets = 1;
            RenderTargetState.DSFormat         = PipelineDsFormat; // DXGI_FORMAT_D32_FLOAT;

            RHISampleState SampleState;
            SampleState.Count = EngineConfig::g_AntialiasingMode == EngineConfig::MSAA ? EngineConfig::g_MASSConfig.m_MSAASampleCount : 1;

            struct PsoStream
            {
                PipelineStateStreamRootSignature     RootSignature;
                PipelineStateStreamInputLayout       InputLayout;
                PipelineStateStreamPrimitiveTopology PrimitiveTopologyType;
                PipelineStateStreamRasterizerState   RasterrizerState;
                PipelineStateStreamVS                VS;
                PipelineStateStreamPS                PS;
                PipelineStateStreamDepthStencilState DepthStencilState;
                PipelineStateStreamRenderTargetState RenderTargetState;
                PipelineStateStreamSampleState       SampleState;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(RootSignatures::pIndirectDraw.get());
            psoStream.InputLayout           = &InputLayout;
            psoStream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
            psoStream.RasterrizerState      = RHIRasterizerState();
            psoStream.VS                    = &Shaders::VS::IndirectDrawVS_BRDF;
            psoStream.PS                    = &Shaders::PS::IndirectDrawPS_BRDF;
            psoStream.DepthStencilState     = DepthStencilState;
            psoStream.RenderTargetState     = RenderTargetState;
            psoStream.SampleState           = SampleState;

            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};
            pIndirectDraw_BRDF = std::make_shared<RHI::D3D12PipelineState>(pDevice, L"IndirectDraw_BRDF", psoDesc);
        }
        {
            //IndirectDrawTransparent
            RHI::D3D12InputLayout InputLayout = MoYu::D3D12MeshVertexPositionNormalTangentTexture::InputLayout;

            RHIDepthStencilState DepthStencilState;
            DepthStencilState.DepthEnable = true;
            DepthStencilState.DepthWrite  = false;
            DepthStencilState.DepthFunc   = RHI_COMPARISON_FUNC::GreaterEqual;

            RHIRenderTargetState RenderTargetState;
            RenderTargetState.RTFormats[0]     = PiplineRtFormat; // DXGI_FORMAT_R32G32B32A32_FLOAT;
            RenderTargetState.NumRenderTargets = 1;
            RenderTargetState.DSFormat         = PipelineDsFormat; // DXGI_FORMAT_D32_FLOAT;

            RHIRenderTargetBlendDesc BlendDesc0;
            BlendDesc0.BlendEnable = true;
            BlendDesc0.SrcBlendRgb = RHI_FACTOR::SrcAlpha;
            BlendDesc0.DstBlendRgb = RHI_FACTOR::OneMinusSrcAlpha;
            BlendDesc0.BlendOpRgb  = RHI_BLEND_OP::Add;

            RHIBlendState BlendState;
            BlendState.RenderTargets[0] = BlendDesc0;

            RHISampleState SampleState;
            SampleState.Count = EngineConfig::g_AntialiasingMode == EngineConfig::MSAA ? EngineConfig::g_MASSConfig.m_MSAASampleCount : 1;

            struct PsoStream
            {
                PipelineStateStreamRootSignature     RootSignature;
                PipelineStateStreamInputLayout       InputLayout;
                PipelineStateStreamPrimitiveTopology PrimitiveTopologyType;
                PipelineStateStreamVS                VS;
                PipelineStateStreamPS                PS;
                PipelineStateStreamBlendState        BlendState;
                PipelineStateStreamDepthStencilState DepthStencilState;
                PipelineStateStreamRenderTargetState RenderTargetState;
                PipelineStateStreamSampleState       SampleState;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(RootSignatures::pIndirectDraw.get());
            psoStream.InputLayout           = &InputLayout;
            psoStream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
            psoStream.VS                    = &Shaders::VS::IndirectDrawVS;
            psoStream.PS                    = &Shaders::PS::IndirectDrawPS;
            psoStream.BlendState            = BlendState;
            psoStream.DepthStencilState     = DepthStencilState;
            psoStream.RenderTargetState     = RenderTargetState;
            psoStream.SampleState           = SampleState;

            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};
            pIndirectDrawTransparent = std::make_shared<RHI::D3D12PipelineState>(pDevice, L"IndirectDrawTransparent", psoDesc);
        }
        {
            RHI::D3D12InputLayout InputLayout = MoYu::D3D12MeshVertexPositionNormalTangentTexture::InputLayout;

            RHIDepthStencilState DepthStencilState;
            DepthStencilState.DepthEnable = true;
            DepthStencilState.DepthFunc   = RHI_COMPARISON_FUNC::GreaterEqual;

            RHIRenderTargetState RenderTargetState;
            RenderTargetState.NumRenderTargets = 0;
            RenderTargetState.DSFormat         = PipelineDsFormat; // DXGI_FORMAT_D32_FLOAT;

            struct PsoStream
            {
                PipelineStateStreamRootSignature     RootSignature;
                PipelineStateStreamInputLayout       InputLayout;
                PipelineStateStreamPrimitiveTopology PrimitiveTopologyType;
                PipelineStateStreamVS                VS;
                PipelineStateStreamPS                PS;
                PipelineStateStreamDepthStencilState DepthStencilState;
                PipelineStateStreamRenderTargetState RenderTargetState;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(RootSignatures::pIndirectDrawDirectionShadowmap.get());
            psoStream.InputLayout           = &InputLayout;
            psoStream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
            psoStream.VS                    = &Shaders::VS::IndirectDrawDirectionShadowmapVS;
            psoStream.DepthStencilState     = DepthStencilState;
            psoStream.RenderTargetState     = RenderTargetState;

            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pIndirectDrawDirectionShadowmap = std::make_shared<RHI::D3D12PipelineState>(pDevice, L"IndirectDrawDirectionShadowmap", psoDesc);
        }
        {
            RHI::D3D12InputLayout InputLayout = MoYu::D3D12MeshVertexPositionNormalTangentTexture::InputLayout;

            RHIDepthStencilState DepthStencilState;
            DepthStencilState.DepthEnable = true;
            DepthStencilState.DepthFunc   = RHI_COMPARISON_FUNC::GreaterEqual;

            RHIRenderTargetState RenderTargetState;
            RenderTargetState.NumRenderTargets = 0;
            RenderTargetState.DSFormat         = PipelineDsFormat; // DXGI_FORMAT_D32_FLOAT;

            struct PsoStream
            {
                PipelineStateStreamRootSignature     RootSignature;
                PipelineStateStreamInputLayout       InputLayout;
                PipelineStateStreamPrimitiveTopology PrimitiveTopologyType;
                PipelineStateStreamVS                VS;
                PipelineStateStreamPS                PS;
                PipelineStateStreamDepthStencilState DepthStencilState;
                PipelineStateStreamRenderTargetState RenderTargetState;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(RootSignatures::pIndirectDrawSpotShadowmap.get());
            psoStream.InputLayout           = &InputLayout;
            psoStream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
            psoStream.VS                    = &Shaders::VS::IndirectDrawSpotShadowmapVS;
            psoStream.DepthStencilState     = DepthStencilState;
            psoStream.RenderTargetState     = RenderTargetState;

            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pIndirectDrawSpotShadowmap = std::make_shared<RHI::D3D12PipelineState>(pDevice, L"IndirectDrawSpotShadowmap", psoDesc);
        }

        {
            RHI::D3D12InputLayout InputLayout = {};

            RHIDepthStencilState DepthStencilState;
            DepthStencilState.DepthEnable = true;
            DepthStencilState.DepthWrite = false;
            DepthStencilState.DepthFunc = RHI_COMPARISON_FUNC::GreaterEqual;

            RHIRenderTargetState RenderTargetState;
            RenderTargetState.RTFormats[0]     = PiplineRtFormat; // DXGI_FORMAT_R32G32B32A32_FLOAT;
            RenderTargetState.NumRenderTargets = 1;
            RenderTargetState.DSFormat         = PipelineDsFormat; // DXGI_FORMAT_D32_FLOAT;

            RHISampleState SampleState;
            SampleState.Count = EngineConfig::g_AntialiasingMode == EngineConfig::MSAA ? EngineConfig::g_MASSConfig.m_MSAASampleCount : 1;

            struct PsoStream
            {
                PipelineStateStreamRootSignature     RootSignature;
                PipelineStateStreamInputLayout       InputLayout;
                PipelineStateStreamPrimitiveTopology PrimitiveTopologyType;
                PipelineStateStreamVS                VS;
                PipelineStateStreamPS                PS;
                PipelineStateStreamDepthStencilState DepthStencilState;
                PipelineStateStreamRenderTargetState RenderTargetState;
                PipelineStateStreamSampleState       SampleState;
            } psoStream;
            psoStream.RootSignature      = PipelineStateStreamRootSignature(RootSignatures::pSkyBoxRootSignature.get());
            psoStream.InputLayout        = &InputLayout;
            psoStream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
            psoStream.VS                    = &Shaders::VS::SkyBoxVS;
            psoStream.PS                    = &Shaders::PS::SkyBoxPS;
            psoStream.DepthStencilState     = DepthStencilState;
            psoStream.RenderTargetState     = RenderTargetState;
            psoStream.SampleState           = SampleState;

            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};
            pSkyBoxPSO = std::make_shared<RHI::D3D12PipelineState>(pDevice, L"SkyBox", psoDesc);
        }
        {
            pGenerateMipsLinearPSO = CreatePSO(RootSignatures::pGenerateMipsLinearSignature.get(), Shaders::CS::GenerateMipsLinearCS, pDevice, L"GenerateMipsLinearPSO");
            pGenerateMipsLinearOddPSO = CreatePSO(RootSignatures::pGenerateMipsLinearSignature.get(), Shaders::CS::GenerateMipsLinearOddCS, pDevice, L"GenerateMipsLinearOddPSO");
            pGenerateMipsLinearOddXPSO = CreatePSO(RootSignatures::pGenerateMipsLinearSignature.get(), Shaders::CS::GenerateMipsLinearOddXCS, pDevice, L"GenerateMipsLinearOddXPSO");
            pGenerateMipsLinearOddYPSO = CreatePSO(RootSignatures::pGenerateMipsLinearSignature.get(), Shaders::CS::GenerateMipsLinearOddYCS, pDevice, L"GenerateMipsLinearOddYPSO");

            pGenerateMaxMipsLinearPSO = CreatePSO(RootSignatures::pGenerateMipsLinearSignature.get(), Shaders::CS::GenerateMaxMipsLinearCS, pDevice, L"GenerateMaxMipsLinearPSO");
            pGenerateMaxMipsLinearOddPSO = CreatePSO(RootSignatures::pGenerateMipsLinearSignature.get(), Shaders::CS::GenerateMaxMipsLinearOddCS, pDevice, L"GenerateMaxMipsLinearOddPSO");
            pGenerateMaxMipsLinearOddXPSO = CreatePSO(RootSignatures::pGenerateMipsLinearSignature.get(), Shaders::CS::GenerateMaxMipsLinearOddXCS, pDevice, L"GenerateMaxMipsLinearOddXPSO");
            pGenerateMaxMipsLinearOddYPSO = CreatePSO(RootSignatures::pGenerateMipsLinearSignature.get(), Shaders::CS::GenerateMaxMipsLinearOddYCS, pDevice, L"GenerateMaxMipsLinearOddYPSO");

            pGenerateMinMipsLinearPSO = CreatePSO(RootSignatures::pGenerateMipsLinearSignature.get(), Shaders::CS::GenerateMinMipsLinearCS, pDevice, L"GenerateMinMipsLinearPSO");
            pGenerateMinMipsLinearOddPSO = CreatePSO(RootSignatures::pGenerateMipsLinearSignature.get(), Shaders::CS::GenerateMinMipsLinearOddCS, pDevice, L"GenerateMinMipsLinearOddPSO");
            pGenerateMinMipsLinearOddXPSO = CreatePSO(RootSignatures::pGenerateMipsLinearSignature.get(), Shaders::CS::GenerateMinMipsLinearOddXCS, pDevice, L"GenerateMinMipsLinearOddXPSO");
            pGenerateMinMipsLinearOddYPSO = CreatePSO(RootSignatures::pGenerateMipsLinearSignature.get(), Shaders::CS::GenerateMinMipsLinearOddYCS, pDevice, L"GenerateMinMipsLinearOddYPSO");
        }
    }

    static void Release()
    {
        pBitonicIndirectArgsPSO = nullptr;
        pBitonic32PreSortPSO    = nullptr;
        pBitonic32InnerSortPSO  = nullptr;
        pBitonic32OuterSortPSO  = nullptr;
        pBitonic64PreSortPSO    = nullptr;
        pBitonic64InnerSortPSO  = nullptr;
        pBitonic64OuterSortPSO  = nullptr;

        pFullScreenPresent              = nullptr;
        pIndirectCullForSort            = nullptr;
        pIndirectCull                   = nullptr;
        pIndirectCullArgs               = nullptr;
        pIndirectCullGrab               = nullptr;
        pIndirectCullDirectionShadowmap = nullptr;
        pIndirectCullSpotShadowmap      = nullptr;

        pIndirectDraw                   = nullptr;
        pIndirectDraw_BRDF              = nullptr;
        pIndirectDrawTransparent        = nullptr;
        pIndirectDrawDirectionShadowmap = nullptr;
        pIndirectDrawSpotShadowmap      = nullptr;

        pSkyBoxPSO = nullptr;

        pGenerateMipsLinearPSO = nullptr;
        pGenerateMipsLinearOddPSO = nullptr;
        pGenerateMipsLinearOddXPSO = nullptr;
        pGenerateMipsLinearOddYPSO = nullptr;

        pGenerateMaxMipsLinearPSO = nullptr;
        pGenerateMaxMipsLinearOddPSO = nullptr;
        pGenerateMaxMipsLinearOddXPSO = nullptr;
        pGenerateMaxMipsLinearOddYPSO = nullptr;

        pGenerateMinMipsLinearPSO = nullptr;
        pGenerateMinMipsLinearOddPSO = nullptr;
        pGenerateMinMipsLinearOddXPSO = nullptr;
        pGenerateMinMipsLinearOddYPSO = nullptr;
    }
};
// clang-format on