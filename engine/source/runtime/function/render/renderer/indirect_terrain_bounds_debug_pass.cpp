#include "runtime/function/render/renderer/indirect_terrain_bounds_debug_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/function/render/renderer/pass_helper.h"
#include "runtime/resource/basic_geometry/cube_mesh.h"
#include <cassert>

namespace MoYu
{

    void IndirectTerrainBoundsDebugPass::prepareMatBuffer(std::shared_ptr<RenderResource> render_resource)
    {
        // MoYu::Geometry::BasicMesh _basicMesh = MoYu::Geometry::CubeMesh::ToBasicMesh();
        // StaticMeshData m_static_mesh_data = MoYu::Geometry::ToStaticMesh(_basicMesh);
        // AABB m_axis_aligned_box = MoYu::Geometry::ToAxisAlignedBox(_basicMesh);
        //
        // InternalIndexBuffer index_buffer = RenderResource::createIndexBuffer<uint32_t>(m_static_mesh_data.m_index_buffer);
        // InternalVertexBuffer vertex_buffer = RenderResource::createVertexBuffer<D3D12MeshVertexPosition>(m_static_mesh_data.m_InputElementDefinition, m_static_mesh_data.m_vertex_buffer);
        
        
    }

	void IndirectTerrainBoundsDebugPass::initialize(const DrawPassInitInfo& init_info)
	{
        colorBufferDesc = init_info.colorTexDesc;
        depthDesc = init_info.depthTexDesc;

        colorBufferDesc.SetFormat(DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT);
        colorBufferDesc.Name = "ColorBuffer";
        depthDesc.Name = "DepthBuffer";

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        terrainBoundsDebugVS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Vertex, m_ShaderRootPath / "pipeline/Runtime/Material/Lit/LightForwardForDebugShader.hlsl", ShaderCompileOptions(L"Vert"));
        terrainBoundsDebugPS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Pixel, m_ShaderRootPath / "pipeline/Runtime/Material/Lit/LightForwardForDebugShader.hlsl", ShaderCompileOptions(L"Frag"));

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                .AddConstantBufferView<0, 0>()
                .AddConstantBufferView<1, 0>()
                .AddDescriptorTable(RHI::D3D12DescriptorTable(1).AddSRVRange<0, 0>(2, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0))
                .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
                .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
                .AddStaticSampler<12, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                .AddStaticSampler<13, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
                .AddStaticSampler<14, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                .AllowInputLayout()
                .AllowResourceDescriptorHeapIndexing()
                .AllowSampleDescriptorHeapIndexing();

            pTerrainBoundDebugSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }
        {
            RHI::CommandSignatureDesc mBuilder(1);
            mBuilder.AddDraw();

            pTerrainBoundDebugCommandSignature = std::make_shared<RHI::D3D12CommandSignature>(
                m_Device, mBuilder, pTerrainBoundDebugSignature->GetApiHandle());
        }

        {
            RHI::D3D12InputLayout InputLayout = {};

            RHIDepthStencilState DepthStencilState;
            DepthStencilState.DepthEnable = true;
            DepthStencilState.DepthWrite = false;
            DepthStencilState.DepthFunc = RHI_COMPARISON_FUNC::GreaterEqual;

            RHIRasterizerState rasterizerState = RHIRasterizerState();
            rasterizerState.CullMode = RHI_CULL_MODE::None;

            RHIRenderTargetState RenderTargetState;
            RenderTargetState.RTFormats[0] = colorBufferDesc.Format;
            RenderTargetState.NumRenderTargets = 1;
            RenderTargetState.DSFormat = DXGI_FORMAT_D32_FLOAT; // DXGI_FORMAT_D32_FLOAT;

            RHISampleState SampleState;
            SampleState.Count = 1;

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
            psoStream.RootSignature = PipelineStateStreamRootSignature(pTerrainBoundDebugSignature.get());
            psoStream.InputLayout = &InputLayout;
            psoStream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
            psoStream.RasterrizerState = rasterizerState;
            psoStream.VS = &terrainBoundsDebugVS;
            psoStream.PS = &terrainBoundsDebugPS;
            psoStream.DepthStencilState = DepthStencilState;
            psoStream.RenderTargetState = RenderTargetState;
            psoStream.SampleState = SampleState;

            PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };
            pTerrainBoundDebugPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"IndirectTerrainDebugBoundsBuffer", psoDesc);
        }
	}

    void IndirectTerrainBoundsDebugPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle terrainConstantBufferHandle = passInput.terrainConstantBufferHandle;
        RHI::RgResourceHandle terrainRenderDataHandle = passInput.terrainRenderDataHandle;
        RHI::RgResourceHandle patchListBufferHandle = passInput.culledPatchListBufferHandle;
        RHI::RgResourceHandle mainCamVisCmdSigHandle = passInput.mainCamVisCmdSigHandle;

        RHI::RenderPass& drawpass = graph.AddRenderPass("IndirectTerrainGBufferPass");

        drawpass.Read(perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        drawpass.Read(terrainConstantBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        drawpass.Read(terrainRenderDataHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(patchListBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(mainCamVisCmdSigHandle, false, RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT, RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT);

        drawpass.Write(passOutput.colorHandle, false, RHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
        drawpass.Write(passOutput.depthHandle, false, RHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE);

        RHI::RgResourceHandle colorHandle = passOutput.colorHandle;
        RHI::RgResourceHandle depthHandle = passOutput.depthHandle;

        drawpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12GraphicsContext* graphicContext = context->GetGraphicsContext();

            RHI::D3D12RenderTargetView* colorView = registry->GetD3D12Texture(colorHandle)->GetDefaultRTV().get();
            RHI::D3D12DepthStencilView* depthStencilView = registry->GetD3D12Texture(depthHandle)->GetDefaultDSV().get();

            graphicContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            graphicContext->SetViewport(RHIViewport {0.0f, 0.0f, (float)colorBufferDesc.Width, (float)colorBufferDesc.Height, 0.0f, 1.0f});
            graphicContext->SetScissorRect(RHIRect {0, 0, (int)colorBufferDesc.Width, (int)colorBufferDesc.Height});

            graphicContext->SetRenderTarget(colorView, depthStencilView);
            //graphicContext->ClearRenderTarget(_rtviews, nullptr);

            graphicContext->SetRootSignature(pTerrainBoundDebugSignature.get());
            graphicContext->SetPipelineState(pTerrainBoundDebugPSO.get());
            
            graphicContext->SetConstantBuffer(0, RegGetBuf(perframeBufferHandle)->GetGpuVirtualAddress());
            graphicContext->SetConstantBuffer(1, RegGetBuf(terrainConstantBufferHandle)->GetGpuVirtualAddress());
            graphicContext->SetDynamicDescriptor(2, 0, RegGetBuf(patchListBufferHandle)->GetDefaultSRV(1)->GetCpuHandle());
            graphicContext->SetDynamicDescriptor(2, 1, RegGetBuf(terrainRenderDataHandle)->GetDefaultSRV(1)->GetCpuHandle());

            auto pMainCamVisCmdSigBuffer = registry->GetD3D12Buffer(mainCamVisCmdSigHandle);

            graphicContext->ExecuteIndirect(pTerrainBoundDebugCommandSignature.get(), pMainCamVisCmdSigBuffer, 0, 1, nullptr, 0);

#ifdef MOYU_RHI_D3D12_DEBUG_RESOURCE_STATES
            LOG_INFO("IndirectTerrainDebugBoundsPass");
#endif
        });

    }

    void IndirectTerrainBoundsDebugPass::destroy()
    {
        pTerrainBoundDebugSignature = nullptr;
        pTerrainBoundDebugPSO       = nullptr;
        pTerrainBoundDebugCommandSignature = nullptr;
    }

}
