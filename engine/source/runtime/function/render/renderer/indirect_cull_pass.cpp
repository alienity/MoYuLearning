#include "runtime/function/render/renderer/indirect_cull_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/core/math/moyu_math.h"

#include <cassert>

namespace Pilot
{
    void IndirectCullPass::initialize(const RenderPassInitInfo& init_info)
    {
        // create default buffer
        pPerframeBuffer = RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                                   RHI::RHIBufferRandomReadWrite,
                                                   1,
                                                   sizeof(HLSL::MeshPerframeStorageBufferObject),
                                                   L"PerFrameBuffer",
                                                   RHI::RHIBufferModeImmutable,
                                                   D3D12_RESOURCE_STATE_GENERIC_READ);

        pMaterialBuffer = RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                                   RHI::RHIBufferRandomReadWrite,
                                                   HLSL::MaterialLimit,
                                                   sizeof(HLSL::MaterialInstance),
                                                   L"MaterialBuffer",
                                                   RHI::RHIBufferModeImmutable,
                                                   D3D12_RESOURCE_STATE_GENERIC_READ);

        pMeshBuffer = RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                               RHI::RHIBufferRandomReadWrite,
                                               HLSL::MeshLimit,
                                               sizeof(HLSL::MeshInstance),
                                               L"MeshBuffer",
                                               RHI::RHIBufferModeImmutable,
                                               D3D12_RESOURCE_STATE_GENERIC_READ);

        // create upload buffer
        pUploadPerframeBuffer = RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                                         RHI::RHIBufferRandomReadWrite,
                                                         1,
                                                         sizeof(HLSL::MeshPerframeStorageBufferObject),
                                                         L"UploadPerFrameBuffer",
                                                         RHI::RHIBufferModeDynamic,
                                                         D3D12_RESOURCE_STATE_GENERIC_READ);

        pUploadMaterialBuffer = RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                                         RHI::RHIBufferRandomReadWrite,
                                                         HLSL::MaterialLimit,
                                                         sizeof(HLSL::MaterialInstance),
                                                         L"UploadMaterialBuffer",
                                                         RHI::RHIBufferModeDynamic,
                                                         D3D12_RESOURCE_STATE_GENERIC_READ);

        pUploadMeshBuffer = RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                                     RHI::RHIBufferRandomReadWrite,
                                                     HLSL::MeshLimit,
                                                     sizeof(HLSL::MeshInstance),
                                                     L"UploadMeshBuffer",
                                                     RHI::RHIBufferModeDynamic,
                                                     D3D12_RESOURCE_STATE_GENERIC_READ);

        // for sort
        int argNumber = 22 * 23 / 2;
        int argSize   = 12;

        pSortDispatchArgs = RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                                     RHI::RHIBufferTargetIndirectArgs,
                                                     argNumber,
                                                     argSize,
                                                     L"SortDispatchArgs",
                                                     RHI::RHIBufferModeImmutable,
                                                     D3D12_RESOURCE_STATE_GENERIC_READ);

        pPerframeObj = pUploadPerframeBuffer->GetCpuVirtualAddress<HLSL::MeshPerframeStorageBufferObject>();
        pMaterialObj = pUploadMaterialBuffer->GetCpuVirtualAddress<HLSL::MaterialInstance>();
        pMeshesObj   = pUploadMeshBuffer->GetCpuVirtualAddress<HLSL::MeshInstance>();

        // buffer for opaque draw
        {
            RHI::RHIBufferTarget opaqueIndexTarget = RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured;

            std::shared_ptr<RHI::D3D12Buffer> opaqueIndexBuffer =
                RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                         opaqueIndexTarget,
                                         HLSL::MeshLimit,
                                         sizeof(HLSL::BitonicSortCommandSigParams),
                                         L"OpaqueIndexBuffer");

            commandBufferForOpaqueDraw.p_IndirectIndexCommandBuffer    = opaqueIndexBuffer;
            commandBufferForOpaqueDraw.p_IndirectIndexCommandBufferSRV = opaqueIndexBuffer->GetDefaultSRV();
            commandBufferForOpaqueDraw.p_IndirectIndexCommandBufferUAV = opaqueIndexBuffer->GetDefaultUAV();

            std::shared_ptr<RHI::D3D12Buffer> opaqueBuffer =
                RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                         opaqueIndexTarget,
                                         HLSL::MeshLimit,
                                         sizeof(HLSL::CommandSignatureParams),
                                         L"OpaqueBuffer");

            commandBufferForOpaqueDraw.p_IndirectSortCommandBuffer    = opaqueBuffer;
            commandBufferForOpaqueDraw.p_IndirectSortCommandBufferUAV = opaqueBuffer->GetDefaultUAV();
        }

        // buffer for transparent draw
        {
            RHI::RHIBufferTarget transparentIndexTarget =
                RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured;

            std::shared_ptr<RHI::D3D12Buffer> transparentIndexBuffer =
                RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                         transparentIndexTarget,
                                         HLSL::MeshLimit,
                                         sizeof(HLSL::BitonicSortCommandSigParams),
                                         L"TransparentIndexBuffer");

            commandBufferForTransparentDraw.p_IndirectIndexCommandBuffer    = transparentIndexBuffer;
            commandBufferForTransparentDraw.p_IndirectIndexCommandBufferSRV = transparentIndexBuffer->GetDefaultSRV();
            commandBufferForTransparentDraw.p_IndirectIndexCommandBufferUAV = transparentIndexBuffer->GetDefaultUAV();

            std::shared_ptr<RHI::D3D12Buffer> transparentBuffer =
                RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                         transparentIndexTarget,
                                         HLSL::MeshLimit,
                                         sizeof(HLSL::CommandSignatureParams),
                                         L"TransparentBuffer");
            
            commandBufferForTransparentDraw.p_IndirectSortCommandBuffer    = transparentBuffer;
            commandBufferForTransparentDraw.p_IndirectSortCommandBufferUAV = transparentBuffer->GetDefaultUAV();
        }
    }

    void IndirectCullPass::prepareMeshData(std::shared_ptr<RenderResourceBase> render_resource)
    {
        RenderResource* real_resource = (RenderResource*)render_resource.get();
        memcpy(pPerframeObj, &real_resource->m_mesh_perframe_storage_buffer_object, sizeof(HLSL::MeshPerframeStorageBufferObject));

        std::vector<RenderMeshNode>& renderMeshNodes = *m_visiable_nodes.p_all_mesh_nodes;

        uint32_t numMeshes = renderMeshNodes.size();
        assert(numMeshes < HLSL::MeshLimit);
        for (size_t i = 0; i < numMeshes; i++)
        {
            RenderMeshNode& temp_node = renderMeshNodes[i];

            std::shared_ptr<RHI::D3D12ShaderResourceView> defaultWhiteView = real_resource->m_default_resource._white_texture2d_image_view;
            std::shared_ptr<RHI::D3D12ShaderResourceView> defaultBlackView = real_resource->m_default_resource._black_texture2d_image_view;

            std::shared_ptr<RHI::D3D12ShaderResourceView> uniformBufferView = temp_node.ref_material->material_uniform_buffer_view;
            std::shared_ptr<RHI::D3D12ShaderResourceView> baseColorView = temp_node.ref_material->base_color_image_view;
            std::shared_ptr<RHI::D3D12ShaderResourceView> metallicRoughnessView = temp_node.ref_material->metallic_roughness_image_view;
            std::shared_ptr<RHI::D3D12ShaderResourceView> normalView   = temp_node.ref_material->normal_image_view;
            std::shared_ptr<RHI::D3D12ShaderResourceView> emissionView = temp_node.ref_material->emissive_image_view;


            HLSL::MaterialInstance curMatInstance = {};
            curMatInstance.uniformBufferViewIndex = uniformBufferView->GetIndex();
            curMatInstance.baseColorViewIndex = baseColorView->IsValid() ? baseColorView->GetIndex() : defaultWhiteView->GetIndex();
            curMatInstance.metallicRoughnessViewIndex = metallicRoughnessView->IsValid() ? metallicRoughnessView->GetIndex() : defaultBlackView->GetIndex();
            curMatInstance.normalViewIndex = normalView->IsValid() ? normalView->GetIndex() : defaultWhiteView->GetIndex();
            curMatInstance.emissionViewIndex = emissionView->IsValid() ? emissionView->GetIndex() : defaultBlackView->GetIndex();

            pMaterialObj[i] = curMatInstance;

            D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments = {};
            drawIndexedArguments.IndexCountPerInstance        = temp_node.ref_mesh->mesh_index_count;
            drawIndexedArguments.InstanceCount                = 1;
            drawIndexedArguments.StartIndexLocation           = 0;
            drawIndexedArguments.BaseVertexLocation           = 0;
            drawIndexedArguments.StartInstanceLocation        = 0;

            HLSL::BoundingBox boundingBox = {};
            boundingBox.center            = temp_node.bounding_box.center;
            boundingBox.extents           = temp_node.bounding_box.extent;

            HLSL::MeshInstance curMeshInstance     = {};
            curMeshInstance.enable_vertex_blending = temp_node.enable_vertex_blending;
            curMeshInstance.model_matrix           = temp_node.model_matrix;
            curMeshInstance.model_matrix_inverse   = temp_node.model_matrix_inverse;
            curMeshInstance.vertexBuffer           = temp_node.ref_mesh->mesh_vertex_buffer.GetVertexBufferView();
            curMeshInstance.indexBuffer            = temp_node.ref_mesh->mesh_index_buffer.GetIndexBufferView();
            curMeshInstance.drawIndexedArguments   = drawIndexedArguments;
            curMeshInstance.boundingBox            = boundingBox;
            curMeshInstance.materialIndex          = i;

            pMeshesObj[i] = curMeshInstance;
        }

        prepareBuffer();
    }

    void IndirectCullPass::prepareBuffer()
    {
        if (m_visiable_nodes.p_directional_light != nullptr && m_visiable_nodes.p_directional_light->m_is_active)
        {
            if (dirShadowmapCommandBuffer.m_gobject_id != m_visiable_nodes.p_directional_light->m_gobject_id ||
                dirShadowmapCommandBuffer.m_gcomponent_id != m_visiable_nodes.p_directional_light->m_gcomponent_id)
            {
                dirShadowmapCommandBuffer.Reset();
            }

            if (dirShadowmapCommandBuffer.p_IndirectSortCommandBuffer == nullptr)
            {
                RHI::RHIBufferTarget indirectCommandTarget =
                    RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter;

                std::shared_ptr<RHI::D3D12Buffer> p_IndirectCommandBuffer =
                    RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                             indirectCommandTarget,
                                             HLSL::MeshLimit,
                                             sizeof(HLSL::CommandSignatureParams),
                                             L"DirectionIndirectSortCommandBuffer");

                dirShadowmapCommandBuffer.m_gobject_id    = m_visiable_nodes.p_directional_light->m_gobject_id;
                dirShadowmapCommandBuffer.m_gcomponent_id = m_visiable_nodes.p_directional_light->m_gcomponent_id;
                dirShadowmapCommandBuffer.p_IndirectSortCommandBuffer    = p_IndirectCommandBuffer;
                dirShadowmapCommandBuffer.p_IndirectSortCommandBufferUAV = p_IndirectCommandBuffer->GetDefaultUAV();
            }
        }
        else
        {
            dirShadowmapCommandBuffer.Reset();
        }

        if (m_visiable_nodes.p_spot_light_list != nullptr)
        {
            int spotLightCount = m_visiable_nodes.p_spot_light_list->size();
            for (size_t i = 0; i < spotLightCount; i++)
            {
                Pilot::SpotLightDesc curSpotLightDesc = m_visiable_nodes.p_spot_light_list->at(i);

                bool curSpotLighBufferExist = false;
                int  curBufferIndex         = -1;
                for (size_t j = 0; j < spotShadowmapCommandBuffer.size(); j++)
                {
                    if (spotShadowmapCommandBuffer[j].m_gobject_id == curSpotLightDesc.m_gobject_id &&
                        spotShadowmapCommandBuffer[j].m_gcomponent_id == curSpotLightDesc.m_gcomponent_id)
                    {
                        curBufferIndex         = j;
                        curSpotLighBufferExist = true;
                        break;
                    }
                }

                if (!curSpotLightDesc.m_is_active && curSpotLighBufferExist)
                {
                    spotShadowmapCommandBuffer[curBufferIndex].Reset();
                    spotShadowmapCommandBuffer.erase(spotShadowmapCommandBuffer.begin() + curBufferIndex);
                }

                if (curSpotLightDesc.m_is_active && !curSpotLighBufferExist)
                {
                    ShadowmapCommandBuffer spotShadowCommandBuffer = {};

                    RHI::RHIBufferTarget indirectCommandTarget =
                        RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter;

                    std::shared_ptr<RHI::D3D12Buffer> p_IndirectCommandBuffer =
                        RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                                 indirectCommandTarget,
                                                 HLSL::MeshLimit,
                                                 sizeof(HLSL::CommandSignatureParams),
                                                 std::wstring(L"SpotIndirectSortCommandBuffer_" + i));

                    spotShadowCommandBuffer.m_lightIndex                   = i;
                    spotShadowCommandBuffer.m_gobject_id                   = curSpotLightDesc.m_gobject_id;
                    spotShadowCommandBuffer.m_gcomponent_id                = curSpotLightDesc.m_gcomponent_id;
                    spotShadowCommandBuffer.p_IndirectSortCommandBuffer    = p_IndirectCommandBuffer;
                    spotShadowCommandBuffer.p_IndirectSortCommandBufferUAV = p_IndirectCommandBuffer->GetDefaultUAV();

                    spotShadowmapCommandBuffer.push_back(spotShadowCommandBuffer);
                }

            }
        }
    }

    void IndirectCullPass::bitonicSort(RHI::D3D12ComputeContext&                      context,
                                       std::shared_ptr<RHI::D3D12Buffer>              keyIndexList,
                                       std::shared_ptr<RHI::D3D12UnorderedAccessView> keyIndexListUAV,
                                       std::shared_ptr<RHI::D3D12Buffer>              countBuffer,
                                       std::shared_ptr<RHI::D3D12ShaderResourceView>  countBufferSRV,
                                       uint32_t                                       counterOffset,
                                       bool                                           isPartiallyPreSorted,
                                       bool                                           sortAscending)
    {
        const uint32_t ElementSizeBytes      = keyIndexList->GetStride();
        const uint32_t MaxNumElements        = 1024; //keyIndexList->GetSizeInBytes() / ElementSizeBytes;
        const uint32_t AlignedMaxNumElements = Pilot::AlignPowerOfTwo(MaxNumElements);
        const uint32_t MaxIterations         = Pilot::Log2(std::max(2048u, AlignedMaxNumElements)) - 10;

        assert(ElementSizeBytes == 4 || ElementSizeBytes == 8, "Invalid key-index list for bitonic sort");

        context.SetRootSignature(RootSignatures::pBitonicSortRootSignature.get());

        // This controls two things.  It is a key that will sort to the end, and it is a mask used to
        // determine whether the current group should sort ascending or descending.
        context.SetConstants(3, counterOffset, sortAscending ? 0xffffffff : 0);
        
        // Generate execute indirect arguments
        context.SetPipelineState(PipelineStates::pBitonicIndirectArgsPSO.get());
        context.TransitionBarrier(countBuffer.get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        context.TransitionBarrier(pSortDispatchArgs.get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        context.SetConstants(0, MaxIterations);
        context->SetComputeRootDescriptorTable(1, countBufferSRV->GetGpuHandle());
        context->SetComputeRootDescriptorTable(2, keyIndexListUAV->GetGpuHandle());
        context.Dispatch(1, 1, 1);

        // Pre-Sort the buffer up to k = 2048.  This also pads the list with invalid indices
        // that will drift to the end of the sorted list.
        context.TransitionBarrier(pSortDispatchArgs.get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
        context.TransitionBarrier(keyIndexList.get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        context.InsertUAVBarrier(keyIndexList.get());
        context.FlushResourceBarriers();

        //context->SetComputeRootUnorderedAccessView(2, keyIndexList->GetGpuVirtualAddress());
        context->SetComputeRootDescriptorTable(2, keyIndexListUAV->GetGpuHandle());

        if (!isPartiallyPreSorted)
        {
            context.SetPipelineState(ElementSizeBytes == 4 ? PipelineStates::pBitonic32PreSortPSO.get() :
                                                             PipelineStates::pBitonic64PreSortPSO.get());
            context->ExecuteIndirect(CommandSignatures::pDispatchIndirectCommandSignature->GetApiHandle(),
                                     1,
                                     pSortDispatchArgs->GetResource(),
                                     0,
                                     pSortDispatchArgs->GetResource(),
                                     counterOffset);
            //context.DispatchIndirect(s_DispatchArgs, 0);
            context.InsertUAVBarrier(keyIndexList.get());
        }

        uint32_t IndirectArgsOffset = 12;

        // We have already pre-sorted up through k = 2048 when first writing our list, so
        // we continue sorting with k = 4096.  For unnecessarily large values of k, these
        // indirect dispatches will be skipped over with thread counts of 0.

        for (uint32_t k = 4096; k <= AlignedMaxNumElements; k *= 2)
        {
            context.SetPipelineState(ElementSizeBytes == 4 ? PipelineStates::pBitonic32OuterSortPSO.get() :
                                                             PipelineStates::pBitonic64OuterSortPSO.get());

            for (uint32_t j = k / 2; j >= 2048; j /= 2)
            {
                context->SetComputeRoot32BitConstant(0, k, j);
                context->ExecuteIndirect(CommandSignatures::pDispatchIndirectCommandSignature->GetApiHandle(),
                                         1,
                                         pSortDispatchArgs->GetResource(),
                                         IndirectArgsOffset,
                                         pSortDispatchArgs->GetResource(),
                                         counterOffset);
                //context.DispatchIndirect(s_DispatchArgs, IndirectArgsOffset);
                context.InsertUAVBarrier(keyIndexList.get());
                IndirectArgsOffset += 12;
            }

            context.SetPipelineState(ElementSizeBytes == 4 ? PipelineStates::pBitonic32InnerSortPSO.get() :
                                                             PipelineStates::pBitonic64InnerSortPSO.get());
            context->ExecuteIndirect(CommandSignatures::pDispatchIndirectCommandSignature->GetApiHandle(),
                                     1,
                                     pSortDispatchArgs->GetResource(),
                                     IndirectArgsOffset,
                                     pSortDispatchArgs->GetResource(),
                                     counterOffset);
            //context.DispatchIndirect(s_DispatchArgs, IndirectArgsOffset);
            context.InsertUAVBarrier(keyIndexList.get());
            IndirectArgsOffset += 12;
        }
    }

    void IndirectCullPass::cullMeshs(RHI::D3D12CommandContext& context,
                                     RHI::RenderGraphRegistry& registry,
                                     IndirectCullOutput&       indirectCullOutput)
    {
        indirectCullOutput.pPerframeBuffer = pPerframeBuffer;
        indirectCullOutput.pMaterialBuffer = pMaterialBuffer;
        indirectCullOutput.pMeshBuffer     = pMeshBuffer;
        indirectCullOutput.p_OpaqueDrawCommandBuffer      = commandBufferForOpaqueDraw.p_IndirectSortCommandBuffer;
        indirectCullOutput.p_TransparentDrawCommandBuffer = commandBufferForTransparentDraw.p_IndirectSortCommandBuffer;
        indirectCullOutput.p_DirShadowmapCommandBuffer    = dirShadowmapCommandBuffer.p_IndirectSortCommandBuffer;
        for (size_t i = 0; i < spotShadowmapCommandBuffer.size(); i++)
        {
            indirectCullOutput.p_SpotShadowmapCommandBuffers.push_back(
                spotShadowmapCommandBuffer[i].p_IndirectSortCommandBuffer);
        }

        int numMeshes = m_visiable_nodes.p_all_mesh_nodes->size();

        RHI::D3D12SyncHandle ComputeSyncHandle;
        if (numMeshes > 0)
        {
            RHI::D3D12CommandContext& copyContext = m_Device->GetLinkedDevice()->GetCopyContext1();
            copyContext.Open();
            {
                copyContext.ResetCounter(commandBufferForOpaqueDraw.p_IndirectIndexCommandBuffer.get(), HLSL::indexCommandBufferCounterOffset);
                copyContext.ResetCounter(commandBufferForOpaqueDraw.p_IndirectSortCommandBuffer.get(), HLSL::commandBufferCounterOffset);
                copyContext.ResetCounter(commandBufferForTransparentDraw.p_IndirectIndexCommandBuffer.get(), HLSL::indexCommandBufferCounterOffset);
                copyContext.ResetCounter(commandBufferForTransparentDraw.p_IndirectSortCommandBuffer.get(), HLSL::commandBufferCounterOffset);
                if (dirShadowmapCommandBuffer.p_IndirectSortCommandBuffer != nullptr)
                {
                    copyContext.ResetCounter(dirShadowmapCommandBuffer.p_IndirectSortCommandBuffer.get(), HLSL::commandBufferCounterOffset);
                }
                for (size_t i = 0; i < spotShadowmapCommandBuffer.size(); i++)
                {
                    copyContext.ResetCounter(spotShadowmapCommandBuffer[i].p_IndirectSortCommandBuffer.get(), HLSL::commandBufferCounterOffset);
                }
                copyContext->CopyResource(pPerframeBuffer->GetResource(), pUploadPerframeBuffer->GetResource());
                copyContext->CopyResource(pMaterialBuffer->GetResource(), pUploadMaterialBuffer->GetResource());
                copyContext->CopyResource(pMeshBuffer->GetResource(), pUploadMeshBuffer->GetResource());
            }
            copyContext.Close();
            RHI::D3D12SyncHandle copySyncHandle = copyContext.Execute(false);

            RHI::D3D12CommandContext& asyncCompute = m_Device->GetLinkedDevice()->GetAsyncComputeCommandContext();
            asyncCompute.GetCommandQueue()->WaitForSyncHandle(copySyncHandle);

            asyncCompute.Open();
            // Opaque and Transparent object cull
            {
                asyncCompute.TransitionBarrier(commandBufferForOpaqueDraw.p_IndirectIndexCommandBuffer.get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                asyncCompute.TransitionBarrier(commandBufferForTransparentDraw.p_IndirectIndexCommandBuffer.get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                asyncCompute.InsertUAVBarrier(commandBufferForOpaqueDraw.p_IndirectIndexCommandBuffer.get());
                asyncCompute.InsertUAVBarrier(commandBufferForTransparentDraw.p_IndirectIndexCommandBuffer.get());

                D3D12ScopedEvent(asyncCompute, "Gpu Frustum Culling for Sort");
                asyncCompute.SetPipelineState(registry.GetPipelineState(PipelineStates::IndirectCullForSort));
                asyncCompute.SetComputeRootSignature(registry.GetRootSignature(RootSignatures::IndirectCullForSort));
                asyncCompute->SetComputeRootConstantBufferView(0, pPerframeBuffer->GetGpuVirtualAddress());
                asyncCompute->SetComputeRootShaderResourceView(1, pMeshBuffer->GetGpuVirtualAddress());
                asyncCompute->SetComputeRootShaderResourceView(2, pMaterialBuffer->GetGpuVirtualAddress());
                asyncCompute->SetComputeRootDescriptorTable(3, commandBufferForOpaqueDraw.p_IndirectIndexCommandBufferUav->GetGpuHandle());
                asyncCompute->SetComputeRootDescriptorTable(4, commandBufferForTransparentDraw.p_IndirectIndexCommandBufferUav->GetGpuHandle());

                asyncCompute.Dispatch1D<128>(numMeshes);
            }
            
            //  Opaque object bitonic sort
            {
                D3D12ScopedEvent(asyncCompute, "Bitonic Sort for opaque");
                bitonicSort(asyncCompute,
                            registry,
                            commandBufferForOpaqueDraw.p_IndirectIndexCommandBuffer,
                            commandBufferForOpaqueDraw.p_IndirectIndexCommandBufferUav,
                            commandBufferForOpaqueDraw.p_IndirectIndexCommandBuffer,
                            commandBufferForOpaqueDraw.p_IndirectIndexCommandBufferSRV,
                            HLSL::commandBufferCounterOffset,
                            true,
                            false);
            }

            // Transparent object bitonic sort
            {
                D3D12ScopedEvent(asyncCompute, "Bitonic Sort for transparent");
                bitonicSort(asyncCompute,
                            registry,
                            commandBufferForTransparentDraw.p_IndirectIndexCommandBuffer,
                            commandBufferForTransparentDraw.p_IndirectIndexCommandBufferUav,
                            commandBufferForTransparentDraw.p_IndirectIndexCommandBuffer,
                            commandBufferForTransparentDraw.p_IndirectIndexCommandBufferSRV,
                            HLSL::commandBufferCounterOffset,
                            true,
                            true);
            }
            
            // Output Object Buffer
            {
                /*
                D3D12ScopedEvent(asyncCompute, "Grabs all objects to render");
                asyncCompute.SetPipelineState(registry.GetPipelineState(PipelineStates::IndirectCullForSort));
                asyncCompute.SetComputeRootSignature(registry.GetRootSignature(RootSignatures::IndirectCullForSort));

                asyncCompute->SetComputeRootConstantBufferView(0, pPerframeBuffer->GetGpuVirtualAddress());
                asyncCompute->SetComputeRootShaderResourceView(1, pMeshBuffer->GetGpuVirtualAddress());
                asyncCompute->SetComputeRootShaderResourceView(2, pMaterialBuffer->GetGpuVirtualAddress());
                asyncCompute->SetComputeRootDescriptorTable(
                    3, commandBufferForOpaqueDraw.p_IndirectIndexCommandBufferUav->GetGpuHandle());
                asyncCompute->SetComputeRootDescriptorTable(
                    4, commandBufferForTransparentDraw.p_IndirectIndexCommandBufferUav->GetGpuHandle());

                asyncCompute.Dispatch1D<128>(numMeshes);

                // Transition to indirect argument state
                asyncCompute.TransitionBarrier(commandBufferForOpaqueDraw.p_IndirectSortCommandBuffer.get(),
                                               D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
                asyncCompute.TransitionBarrier(commandBufferForTransparentDraw.p_IndirectSortCommandBuffer.get(),
                                               D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
                */
            }

            // DirectionLight shadow cull
            if (dirShadowmapCommandBuffer.p_IndirectSortCommandBuffer != nullptr)
            {
                D3D12ScopedEvent(asyncCompute, "Gpu Frustum Culling direction light");
                asyncCompute.SetPipelineState(registry.GetPipelineState(PipelineStates::IndirectCullDirectionShadowmap));
                asyncCompute.SetComputeRootSignature(registry.GetRootSignature(RootSignatures::IndirectCullDirectionShadowmap));
                
                asyncCompute->SetComputeRootConstantBufferView(0, pPerframeBuffer->GetGpuVirtualAddress());
                asyncCompute->SetComputeRootShaderResourceView(1,pMeshBuffer->GetGpuVirtualAddress());
                asyncCompute->SetComputeRootShaderResourceView(2, pMaterialBuffer->GetGpuVirtualAddress());
                asyncCompute->SetComputeRootDescriptorTable(
                    3, dirShadowmapCommandBuffer.p_IndirectSortCommandBufferUav->GetGpuHandle());

                asyncCompute.Dispatch1D<128>(numMeshes);

                asyncCompute.TransitionBarrier(dirShadowmapCommandBuffer.p_IndirectSortCommandBuffer.get(),
                                               D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
            }

            // SpotLight shadow cull
            if (!spotShadowmapCommandBuffer.empty())
            {
                D3D12ScopedEvent(asyncCompute, "Gpu Frustum Culling spot light");
                for (size_t i = 0; i < spotShadowmapCommandBuffer.size(); i++)
                {
                    asyncCompute.SetPipelineState(registry.GetPipelineState(PipelineStates::IndirectCullSpotShadowmap));
                    asyncCompute.SetComputeRootSignature(registry.GetRootSignature(RootSignatures::IndirectCullSpotShadowmap));

                    asyncCompute->SetComputeRootConstantBufferView(0, pPerframeBuffer->GetGpuVirtualAddress());
                    asyncCompute->SetComputeRoot32BitConstant(1, spotShadowmapCommandBuffer[i].m_lightIndex, 0);
                    asyncCompute->SetComputeRootShaderResourceView(2, pMeshBuffer->GetGpuVirtualAddress());
                    asyncCompute->SetComputeRootShaderResourceView(3, pMaterialBuffer->GetGpuVirtualAddress());
                    asyncCompute->SetComputeRootDescriptorTable(
                        4, spotShadowmapCommandBuffer[i].p_IndirectSortCommandBufferUav->GetGpuHandle());

                    asyncCompute.Dispatch1D<128>(numMeshes);

                    asyncCompute.TransitionBarrier(spotShadowmapCommandBuffer[i].p_IndirectSortCommandBuffer.get(),
                                                   D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
                }
            }
            asyncCompute.Close();
            ComputeSyncHandle = asyncCompute.Execute(false);
        }

        context.GetCommandQueue()->WaitForSyncHandle(ComputeSyncHandle);

    }

    void IndirectCullPass::destroy()
    {
        pUploadPerframeBuffer = nullptr;
        pUploadMaterialBuffer = nullptr;
        pUploadMeshBuffer     = nullptr;

        pPerframeBuffer = nullptr;
        pMaterialBuffer = nullptr;
        pMeshBuffer     = nullptr;

        commandBufferForOpaqueDraw.ResetBuffer();
        commandBufferForTransparentDraw.ResetBuffer();
        dirShadowmapCommandBuffer.Reset();
        for (size_t i = 0; i < spotShadowmapCommandBuffer.size(); i++)
            spotShadowmapCommandBuffer[i].Reset();
        spotShadowmapCommandBuffer.clear();

    }

}
