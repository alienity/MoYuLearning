#include "d3d12_commandContext.h"
#include "d3d12_device.h"
#include "d3d12_linkedDevice.h"
#include "d3d12_commandQueue.h"
#include "d3d12_descriptor.h"
#include "d3d12_graphicsCommon.h"
#include "d3d12_graphicsMemory.h"
#include "d3d12_resourceUploadBatch.h"
#include "function/render/utility/HDUtils.h"

#include "runtime/core/base/utility.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/global/global_context.h"
#include "runtime/core/math/moyu_math2.h"

namespace RHI
{
    // D3D12CommandAllocatorPool
    D3D12CommandAllocatorPool::D3D12CommandAllocatorPool(D3D12LinkedDevice* Parent, D3D12_COMMAND_LIST_TYPE CommandListType) noexcept :
        D3D12LinkedDeviceChild(Parent),
        m_CommandListType(CommandListType)
    {}

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> D3D12CommandAllocatorPool::RequestCommandAllocator()
    {
        auto CreateCommandAllocator = [this] {
            Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator;
            VERIFY_D3D12_API(GetParentLinkedDevice()->GetDevice()->CreateCommandAllocator(
                m_CommandListType, IID_PPV_ARGS(CommandAllocator.ReleaseAndGetAddressOf())));
            return CommandAllocator;
        };
        auto CommandAllocator = m_CommandAllocatorPool.RetrieveFromPool(CreateCommandAllocator);
        CommandAllocator->Reset();
        return CommandAllocator;
    }

	void
    D3D12CommandAllocatorPool::DiscardCommandAllocator(Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& CommandAllocator, D3D12SyncHandle SyncHandle)
    {
        m_CommandAllocatorPool.ReturnToPool(std::move(CommandAllocator), SyncHandle);
    }

	// D3D12CommandContext
    D3D12CommandContext::D3D12CommandContext(D3D12LinkedDevice* Parent, RHID3D12CommandQueueType Type, D3D12_COMMAND_LIST_TYPE CommandListType) :
        D3D12LinkedDeviceChild(Parent),
        m_Type(Type), m_CommandListType(CommandListType), m_CommandListHandle(Parent, CommandListType),
        m_CommandAllocator(nullptr), m_CommandAllocatorPool(Parent, CommandListType), m_GraphicsMemory(Parent->GetGraphicsMemory())
    {
        m_pDynamicViewDescriptorHeap = std::make_shared<DynamicDescriptorHeap>(Parent, this, Parent->GetResourceDescriptorHeap());
        m_pDynamicSamplerDescriptorHeap = std::make_shared<DynamicDescriptorHeap>(Parent, this, Parent->GetSamplerDescriptorHeap());
    }

    D3D12CommandContext::~D3D12CommandContext()
    {
        assert(!m_CommandListHandle.IsRecording());
    }

	D3D12CommandQueue* D3D12CommandContext::GetCommandQueue() const noexcept
    {
        return GetParentLinkedDevice()->GetCommandQueue(m_Type);
    }

	ID3D12GraphicsCommandList* D3D12CommandContext::GetGraphicsCommandList() const noexcept
    {
        return m_CommandListHandle.GetGraphicsCommandList();
    }

	ID3D12GraphicsCommandList4* D3D12CommandContext::GetGraphicsCommandList4() const noexcept
    {
        return m_CommandListHandle.GetGraphicsCommandList4();
    }

	ID3D12GraphicsCommandList6* D3D12CommandContext::GetGraphicsCommandList6() const noexcept
    {
        return m_CommandListHandle.GetGraphicsCommandList6();
    }

    D3D12GraphicsContext* D3D12CommandContext::GetGraphicsContext()
    {
        ASSERT(m_CommandListType != D3D12_COMMAND_LIST_TYPE_COMPUTE);
        return reinterpret_cast<D3D12GraphicsContext*>(this);
    }

    D3D12ComputeContext* D3D12CommandContext::GetComputeContext()
    {
        return reinterpret_cast<D3D12ComputeContext*>(this);
    }

    std::shared_ptr<D3D12CommandContext> D3D12CommandContext::Begin(D3D12LinkedDevice* Parent, RHID3D12CommandQueueType Type)
    {
        std::shared_ptr<D3D12CommandContext> _CommandContext = std::make_shared<D3D12CommandContext>(Parent, Type, D3D12_COMMAND_LIST_TYPE_DIRECT);
        _CommandContext->Open();
        return _CommandContext;
    }

    D3D12SyncHandle D3D12CommandContext::Finish(bool WaitForCompletion)
    {
        this->Close();
        D3D12SyncHandle _syncHandle = this->Execute(WaitForCompletion);
        return _syncHandle;
    }

    void D3D12CommandContext::Open()
    {
        if (!m_CommandAllocator)
        {
            m_CommandAllocator = m_CommandAllocatorPool.RequestCommandAllocator();
        }
        bool isCommandListNew = m_CommandListHandle.Open(m_CommandAllocator.Get());
        if (!isCommandListNew)
            return;

        if (m_Type == RHID3D12CommandQueueType::Direct || m_Type == RHID3D12CommandQueueType::AsyncCompute || m_Type == RHID3D12CommandQueueType::Copy2)
        {
            ID3D12DescriptorHeap* DescriptorHeaps[2] = {
                GetParentLinkedDevice()->GetResourceDescriptorHeap()->GetDescriptorHeap(),
                GetParentLinkedDevice()->GetSamplerDescriptorHeap()->GetDescriptorHeap(),
            };

            m_CommandListHandle->SetDescriptorHeaps(2, DescriptorHeaps);
        }

        // Reset cache
        Cache = {};
    }

    void D3D12CommandContext::Close()
    {
        m_CommandListHandle.Close();
    }

	D3D12SyncHandle D3D12CommandContext::Execute(bool WaitForCompletion)
    {
        D3D12SyncHandle SyncHandle = GetCommandQueue()->ExecuteCommandLists({&m_CommandListHandle}, WaitForCompletion);

        // Release dynamic descriptors
        m_pDynamicViewDescriptorHeap->CleanupUsedHeaps();
        m_pDynamicSamplerDescriptorHeap->CleanupUsedHeaps();
        // Release the command allocator so it can be reused.
        m_CommandAllocatorPool.DiscardCommandAllocator(m_CommandAllocator, SyncHandle);
        // Release temp resourcce
        //m_CpuLinearAllocator.Version(SyncHandle);
        m_GraphicsMemory->Commit(SyncHandle);

#ifdef MOYU_RHI_D3D12_DEBUG_RESOURCE_STATES
        LOG_INFO("=============== D3D12CommandContext Execute ===============");
#endif
        return SyncHandle;
    }

    void D3D12CommandContext::CopyBuffer(D3D12Buffer* Dest, D3D12Buffer* Src)
    {
        if (Src->GetBufferDesc().mode == RHIBufferMode::RHIBufferModeImmutable)
            TransitionBarrier(Src, D3D12_RESOURCE_STATE_COPY_SOURCE);
        TransitionBarrier(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
        FlushResourceBarriers();
        m_CommandListHandle->CopyResource(Dest->GetResource(), Src->GetResource());

#ifdef MOYU_RHI_D3D12_DEBUG_RESOURCE_STATES
        LOG_INFO("CopyBuffer from {} to {}", MoYu::HDUtils::ws2s(Src->GetResourceName()), MoYu::HDUtils::ws2s(Dest->GetResourceName()));
#endif
    }

    void D3D12CommandContext::CopyBufferRegion(D3D12Buffer* Dest, UINT64 DestOffset, D3D12Buffer* Src, UINT64 SrcOffset, UINT64 NumBytes)
    {
        if (Src->GetBufferDesc().mode == RHIBufferMode::RHIBufferModeImmutable)
            TransitionBarrier(Src, D3D12_RESOURCE_STATE_COPY_SOURCE);
        TransitionBarrier(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
        FlushResourceBarriers();
        m_CommandListHandle->CopyBufferRegion(Dest->GetResource(), DestOffset, Src->GetResource(), SrcOffset, NumBytes);

#ifdef MOYU_RHI_D3D12_DEBUG_RESOURCE_STATES
        LOG_INFO("CopyBufferRegion from {} offset {} to {} offset {} numbytes {}", MoYu::HDUtils::ws2s(Src->GetResourceName()), SrcOffset, MoYu::HDUtils::ws2s(Dest->GetResourceName()), DestOffset, NumBytes);
#endif
    }

    void D3D12CommandContext::CopySubresource(D3D12Texture* Dest, uint32_t DestSubIndex, D3D12Texture* Src, uint32_t SrcSubIndex)
    {
        FlushResourceBarriers();
        D3D12_TEXTURE_COPY_LOCATION DestLocation = {Dest->GetResource(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, DestSubIndex};
        D3D12_TEXTURE_COPY_LOCATION SrcLocation = {Src->GetResource(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, SrcSubIndex};
        m_CommandListHandle->CopyTextureRegion(&DestLocation, 0, 0, 0, &SrcLocation, nullptr);

#ifdef MOYU_RHI_D3D12_DEBUG_RESOURCE_STATES
        LOG_INFO("CopySubresource from {} subindex {} to {} subindex {}", MoYu::HDUtils::ws2s(Src->GetResourceName()), SrcSubIndex, MoYu::HDUtils::ws2s(Dest->GetResourceName()), DestSubIndex);
#endif
    }

    void D3D12CommandContext::CopyTextureRegion(D3D12Texture* Dest, uint32_t x, uint32_t y, uint32_t z, D3D12Texture* Source, RECT& Rect)
    {
        TransitionBarrier(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
        TransitionBarrier(Source, D3D12_RESOURCE_STATE_COPY_SOURCE);
        FlushResourceBarriers();

        D3D12_TEXTURE_COPY_LOCATION destLoc = CD3DX12_TEXTURE_COPY_LOCATION(Dest->GetResource(), 0);
        D3D12_TEXTURE_COPY_LOCATION srcLoc  = CD3DX12_TEXTURE_COPY_LOCATION(Source->GetResource(), 0);

        D3D12_BOX box = {};
        box.back      = 1;
        box.left      = Rect.left;
        box.right     = Rect.right;
        box.top       = Rect.top;
        box.bottom    = Rect.bottom;

        m_CommandListHandle->CopyTextureRegion(&destLoc, x, y, z, &srcLoc, &box);

#ifdef MOYU_RHI_D3D12_DEBUG_RESOURCE_STATES
        LOG_INFO("CopyTextureRegion from {} rect(left, right, top, down) ({}, {}, {}, {}) to {} (DstX, DstY, DstZ) ({}, {}, {})",
            MoYu::HDUtils::ws2s(Source->GetResourceName()), Rect.left, Rect.right, Rect.top, Rect.bottom,
            MoYu::HDUtils::ws2s(Dest->GetResourceName()), x, y, z);
#endif
    }

    void D3D12CommandContext::ResetCounter(D3D12Buffer* CounterResource, UINT64 CounterOffset, uint32_t Value /*= 0*/)
    {
        FillBuffer(CounterResource, 0, Value, sizeof(uint32_t));
        //TransitionBarrier(CounterResource, D3D12_RESOURCE_STATE_GENERIC_READ);
#ifdef MOYU_RHI_D3D12_DEBUG_RESOURCE_STATES
        LOG_INFO("ResetCounter {} Value {}", MoYu::HDUtils::ws2s(CounterResource->GetResourceName()), Value);
#endif
    }

    GraphicsResource D3D12CommandContext::ReserveUploadMemory(UINT64 SizeInBytes, uint32_t Alignment)
    {
        return m_GraphicsMemory->Allocate(SizeInBytes, Alignment);
    }

    std::shared_ptr<D3D12Buffer> D3D12CommandContext::ReverveTmpBuffer(UINT64 SizeInBytes)
    {
        return nullptr;
    }

    std::shared_ptr<D3D12Texture> D3D12CommandContext::ReverveTmpTexture(RHIRenderSurfaceBaseDesc SurfaceDesc)
    {
        return nullptr;
    }

    void D3D12CommandContext::WriteBuffer(D3D12Buffer* Dest, UINT64 DestOffset, const void* BufferData, UINT64 NumBytes)
    {
        ASSERT(BufferData != nullptr && MoYu::IsAligned(BufferData, 16));
        GraphicsResource TempSpace = m_GraphicsMemory->Allocate(NumBytes, 512);
        SIMDMemCopy(TempSpace.Memory(), BufferData, MoYu::DivideByMultiple(NumBytes, 16));
        TransitionBarrier(Dest, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
        m_CommandListHandle->CopyBufferRegion(Dest->GetResource(), DestOffset, TempSpace.Resource(), TempSpace.ResourceOffset(), NumBytes);

#ifdef MOYU_RHI_D3D12_DEBUG_RESOURCE_STATES
        LOG_INFO("WriteBuffer {}", MoYu::HDUtils::ws2s(Dest->GetResourceName()));
#endif
    }

    void D3D12CommandContext::FillBuffer(D3D12Buffer* Dest, UINT64 DestOffset, DWParam Value, UINT64 NumBytes)
    {
        GraphicsResource TempSpace   = m_GraphicsMemory->Allocate(NumBytes, 512);
        __m128   VectorValue = _mm_set1_ps(Value.Float);
        SIMDMemFill(TempSpace.Memory(), VectorValue, MoYu::DivideByMultiple(NumBytes, 16));
        TransitionBarrier(Dest, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
        m_CommandListHandle->CopyBufferRegion(Dest->GetResource(), DestOffset, TempSpace.Resource(), TempSpace.ResourceOffset(), NumBytes);

#ifdef MOYU_RHI_D3D12_DEBUG_RESOURCE_STATES
        LOG_INFO("FillBuffer {}", MoYu::HDUtils::ws2s(Dest->GetResourceName()));
#endif
    }

    void D3D12CommandContext::TransitionBarrier(D3D12Resource* Resource, D3D12_RESOURCE_STATES State, uint32_t Subresource, bool FlushImmediate)
    {
        if (Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
        {
            const CResourceState& allTrackedState = m_CommandListHandle.GetAllTrackedResourceState(Resource);
            if (allTrackedState.IsUniform())
            {
                D3D12_RESOURCE_STATES trackedResourceState = allTrackedState.GetSubresourceState(0);
                if (trackedResourceState != State)
                {
                    m_CommandListHandle.TransitionBarrier(Resource, State, Subresource);
                }
            }
            else
            {
                for (int i = 0; i < allTrackedState.SubresourceStates.size(); i++)
                {
                    if (allTrackedState.SubresourceStates[i] != State)
                    {
                        m_CommandListHandle.TransitionBarrier(Resource, State, i);
                    }
                }
            }
        }
        else
        {
            m_CommandListHandle.TransitionBarrier(Resource, State, Subresource);
        }
        if (State == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        {
            m_CommandListHandle.UAVBarrier(Resource);
        }
        if (FlushImmediate)
        {
            m_CommandListHandle.FlushResourceBarriers();
        }

        //D3D12_RESOURCE_STATES trackedResourceState = m_CommandListHandle.GetResourceStateTracked(Resource, Subresource);
        //if (trackedResourceState != State)
        //{
        //    m_CommandListHandle.TransitionBarrier(Resource, State, Subresource);
        //    if (/*trackedResourceState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS ||*/
        //        State == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        //    {
        //        m_CommandListHandle.UAVBarrier(Resource);
        //    }
        //}
        //if (FlushImmediate)
        //    m_CommandListHandle.FlushResourceBarriers();
    }

    void D3D12CommandContext::AliasingBarrier(D3D12Resource* BeforeResource, D3D12Resource* AfterResource, bool FlushImmediate)
    {
        m_CommandListHandle.AliasingBarrier(BeforeResource, AfterResource);

        if (FlushImmediate)
            m_CommandListHandle.FlushResourceBarriers();
    }

    void D3D12CommandContext::InsertUAVBarrier(D3D12Resource* Resource, bool FlushImmediate)
    {
        m_CommandListHandle.UAVBarrier(Resource);

        if (FlushImmediate)
            m_CommandListHandle.FlushResourceBarriers();
    }

	void D3D12CommandContext::FlushResourceBarriers()
    {
        m_CommandListHandle.FlushResourceBarriers();
    }

	bool D3D12CommandContext::AssertResourceState(D3D12Resource* Resource, D3D12_RESOURCE_STATES State, uint32_t Subresource)
    {
        return m_CommandListHandle.AssertResourceState(Resource, State, Subresource);
    }

	void D3D12CommandContext::SetPipelineState(D3D12PipelineState* PipelineState)
    {
        if (Cache.PipelineState != PipelineState)
        {
            Cache.PipelineState = PipelineState;

            m_CommandListHandle->SetPipelineState(Cache.PipelineState->GetApiHandle());
        }
    }

	void D3D12CommandContext::SetPipelineState(D3D12RaytracingPipelineState* RaytracingPipelineState)
    {
        if (Cache.Raytracing.RaytracingPipelineState != RaytracingPipelineState)
        {
            Cache.Raytracing.RaytracingPipelineState = RaytracingPipelineState;

            m_CommandListHandle.GetGraphicsCommandList4()->SetPipelineState1(RaytracingPipelineState->GetApiHandle());
        }
    }

    void D3D12CommandContext::SetPredication(D3D12Buffer* Buffer, UINT64 BufferOffset, D3D12_PREDICATION_OP Op)
    {
        m_CommandListHandle->SetPredication(Buffer->GetResource(), BufferOffset, Op);
    }

    void D3D12CommandContext::DispatchRays(const D3D12_DISPATCH_RAYS_DESC* pDesc)
    {
        m_CommandListHandle.FlushResourceBarriers();
        m_CommandListHandle.GetGraphicsCommandList4()->DispatchRays(pDesc);
    }

    void D3D12CommandContext::DispatchMesh(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ)
    {
        m_CommandListHandle.FlushResourceBarriers();
        m_CommandListHandle.GetGraphicsCommandList6()->DispatchMesh(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
    }

    /*
    template<RHI_PIPELINE_STATE_TYPE PsoType>
    void D3D12CommandContext::SetDynamicResourceDescriptorTables(D3D12RootSignature* RootSignature)
    {
        ID3D12GraphicsCommandList* m_CommandList = m_CommandListHandle.GetGraphicsCommandList();

        // Bindless descriptors
        uint32_t NumParameters      = RootSignature->GetNumParameters();
        uint32_t Offset             = NumParameters - RootParameters::DescriptorTable::NumRootParameters;
        auto ResourceDescriptor = GetParentLinkedDevice()->GetResourceDescriptorHeap().GetGpuDescriptorHandle(0);
        auto SamplerDescriptor  = GetParentLinkedDevice()->GetSamplerDescriptorHeap().GetGpuDescriptorHandle(0);

        (m_CommandList->*D3D12DescriptorTableTraits<PsoType>::Bind())(RootParameters::DescriptorTable::ShaderResourceDescriptorTable + Offset, ResourceDescriptor);
        (m_CommandList->*D3D12DescriptorTableTraits<PsoType>::Bind())(RootParameters::DescriptorTable::UnorderedAccessDescriptorTable + Offset, ResourceDescriptor);
        (m_CommandList->*D3D12DescriptorTableTraits<PsoType>::Bind())(RootParameters::DescriptorTable::SamplerDescriptorTable + Offset, SamplerDescriptor);
    }
    */
    // ================================Graphic=====================================

    void D3D12GraphicsContext::ClearUAV(RHI::D3D12Buffer* Target)
    {
        FlushResourceBarriers();

        std::shared_ptr<D3D12UnorderedAccessView> uav = Target->GetDefaultUAV();
        const uint32_t ClearColor[4] = {};
        m_CommandListHandle->ClearUnorderedAccessViewUint(
            uav->GetGpuHandle(), uav->GetCpuHandle(), Target->GetResource(), ClearColor, 0, nullptr);
    }

    void D3D12GraphicsContext::ClearUAV(RHI::D3D12Texture* Target)
    {
        FlushResourceBarriers();

        std::shared_ptr<D3D12UnorderedAccessView> uav = Target->GetDefaultUAV();
        CD3DX12_RECT clearRect(0, 0, (LONG)Target->GetWidth(), (LONG)Target->GetHeight());
        D3D12_CLEAR_VALUE clearVal = Target->GetClearValue();
        m_CommandListHandle->ClearUnorderedAccessViewFloat(
            uav->GetGpuHandle(), uav->GetCpuHandle(), Target->GetResource(), clearVal.Color, 1, &clearRect);
    }

    void D3D12GraphicsContext::ClearColor(RHI::D3D12Texture* Target, D3D12_RECT* Rect)
    {
        FlushResourceBarriers();

        m_CommandListHandle->ClearRenderTargetView(
            Target->GetDefaultRTV()->GetCpuHandle(), Target->GetClearValue().Color, (Rect == nullptr) ? 0 : 1, Rect);
    }

    void D3D12GraphicsContext::ClearColor(RHI::D3D12Texture* Target, float Colour[4], D3D12_RECT* Rect)
    {
        FlushResourceBarriers();

        m_CommandListHandle->ClearRenderTargetView(
            Target->GetDefaultRTV()->GetCpuHandle(), Colour, (Rect == nullptr) ? 0 : 1, Rect);
    }

    void D3D12GraphicsContext::ClearDepth(RHI::D3D12Texture* Target)
    {
        FlushResourceBarriers();
        m_CommandListHandle->ClearDepthStencilView(Target->GetDefaultDSV()->GetCpuHandle(),
                                                   D3D12_CLEAR_FLAG_DEPTH,
                                                   Target->GetClearDepth(),
                                                   Target->GetClearStencil(),
                                                   0,
                                                   nullptr);
    }

    void D3D12GraphicsContext::ClearStencil(RHI::D3D12Texture* Target)
    {
        FlushResourceBarriers();
        m_CommandListHandle->ClearDepthStencilView(Target->GetDefaultDSV()->GetCpuHandle(),
                                                   D3D12_CLEAR_FLAG_STENCIL,
                                                   Target->GetClearDepth(),
                                                   Target->GetClearStencil(),
                                                   0,
                                                   nullptr);
    }

    void D3D12GraphicsContext::ClearDepthAndStencil(RHI::D3D12Texture* Target)
    {
        FlushResourceBarriers();
        m_CommandListHandle->ClearDepthStencilView(Target->GetDefaultDSV()->GetCpuHandle(),
                                                   D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                                                   Target->GetClearDepth(),
                                                   Target->GetClearStencil(),
                                                   0,
                                                   nullptr);
    }

    void D3D12GraphicsContext::ClearRenderTarget(std::vector<D3D12RenderTargetView*> RenderTargetViews, D3D12DepthStencilView* DepthStencilView)
    {
        // Transition
        for (auto& RenderTargetView : RenderTargetViews)
        {
            auto& ViewSubresourceSubset = RenderTargetView->GetViewSubresourceSubset();
            for (auto Iter = ViewSubresourceSubset.begin(); Iter != ViewSubresourceSubset.end(); ++Iter)
            {
                for (uint32_t SubresourceIndex = Iter.StartSubresource(); SubresourceIndex < Iter.EndSubresource(); ++SubresourceIndex)
                {
                    TransitionBarrier(
                        RenderTargetView->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, SubresourceIndex);
                }
            }
        }
        if (DepthStencilView)
        {
            auto& ViewSubresourceSubset = DepthStencilView->GetViewSubresourceSubset();
            for (auto Iter = ViewSubresourceSubset.begin(); Iter != ViewSubresourceSubset.end(); ++Iter)
            {
                for (uint32_t SubresourceIndex = Iter.StartSubresource(); SubresourceIndex < Iter.EndSubresource(); ++SubresourceIndex)
                {
                    TransitionBarrier(
                        DepthStencilView->GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, SubresourceIndex);
                }
            }
        }

        // Flush
        FlushResourceBarriers();

        // Clear
        for (auto& RenderTargetView : RenderTargetViews)
        {
            D3D12_CLEAR_VALUE ClearValue = RenderTargetView->GetResource()->GetClearValue();
            m_CommandListHandle->ClearRenderTargetView(RenderTargetView->GetCpuHandle(), ClearValue.Color, 0, nullptr);
        }
        if (DepthStencilView)
        {
            D3D12_CLEAR_VALUE ClearValue = DepthStencilView->GetResource()->GetClearValue();
            FLOAT             Depth      = ClearValue.DepthStencil.Depth;
            UINT8             Stencil    = ClearValue.DepthStencil.Stencil;

            m_CommandListHandle->ClearDepthStencilView(DepthStencilView->GetCpuHandle(),
                                                       D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                                                       Depth,
                                                       Stencil,
                                                       0,
                                                       nullptr);
        }
    }

    void D3D12GraphicsContext::ClearRenderTarget(D3D12RenderTargetView* RenderTargetView, D3D12DepthStencilView* DepthStencilView)
    {
        std::vector<D3D12RenderTargetView*> rtvs = {};
        if (RenderTargetView != nullptr)
            rtvs.push_back(RenderTargetView);
        this->ClearRenderTarget(rtvs, DepthStencilView);
    }

    void D3D12GraphicsContext::BeginQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, uint32_t HeapIndex)
    {
        m_CommandListHandle->BeginQuery(QueryHeap, Type, HeapIndex);
    }

    void D3D12GraphicsContext::EndQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, uint32_t HeapIndex)
    {
        m_CommandListHandle->EndQuery(QueryHeap, Type, HeapIndex);
    }

    void D3D12GraphicsContext::ResolveQueryData(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, uint32_t StartIndex, uint32_t NumQueries, ID3D12Resource* DestinationBuffer, UINT64 DestinationBufferOffset)
    {
        m_CommandListHandle->ResolveQueryData(
            QueryHeap, Type, StartIndex, NumQueries, DestinationBuffer, DestinationBufferOffset);
    }

	void D3D12GraphicsContext::SetRootSignature(D3D12RootSignature* RootSignature)
    {
        if (Cache.RootSignature == RootSignature)
            return;

        m_CommandListHandle->SetGraphicsRootSignature(RootSignature->GetApiHandle());

        m_pDynamicViewDescriptorHeap->ParseGraphicsRootSignature(RootSignature);
        m_pDynamicSamplerDescriptorHeap->ParseGraphicsRootSignature(RootSignature);

        /*
        if (Cache.RootSignature != RootSignature)
        {
            Cache.RootSignature = RootSignature;

            m_CommandListHandle->SetGraphicsRootSignature(RootSignature->GetApiHandle());

            SetDynamicResourceDescriptorTables<RHI_PIPELINE_STATE_TYPE::Graphics>(RootSignature);
        }
        */
    }

    void D3D12GraphicsContext::SetRenderTargets(std::vector<D3D12RenderTargetView*> RenderTargetViews)
    {
        SetRenderTargets(RenderTargetViews, nullptr);
    }

    void D3D12GraphicsContext::SetRenderTargets(std::vector<D3D12RenderTargetView*> RenderTargetViews, D3D12DepthStencilView* DepthStencilView)
    {
        uint32_t                        NumRenderTargetDescriptors = static_cast<uint32_t>(RenderTargetViews.size());
        D3D12_CPU_DESCRIPTOR_HANDLE pRenderTargetDescriptors[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
        D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilDescriptor                                           = {};
        for (uint32_t i = 0; i < NumRenderTargetDescriptors; ++i)
        {
            pRenderTargetDescriptors[i] = RenderTargetViews[i]->GetCpuHandle();
        }
        if (DepthStencilView)
        {
            DepthStencilDescriptor = DepthStencilView->GetCpuHandle();
        }
        m_CommandListHandle->OMSetRenderTargets(NumRenderTargetDescriptors,
                                                pRenderTargetDescriptors,
                                                FALSE,
                                                DepthStencilView ? &DepthStencilDescriptor : nullptr);
    }

    void D3D12GraphicsContext::SetRenderTarget(D3D12RenderTargetView* RenderTargetView)
    {
        SetRenderTarget(RenderTargetView, nullptr);
    }

    void D3D12GraphicsContext::SetRenderTarget(D3D12RenderTargetView* RenderTargetView, D3D12DepthStencilView* DepthStencilView)
    {
        std::vector<D3D12RenderTargetView*> rtvs = {};
        if (RenderTargetView != nullptr)
            rtvs.push_back(RenderTargetView);
        this->SetRenderTargets(rtvs, DepthStencilView);
    }

    void D3D12GraphicsContext::SetViewport(const RHIViewport& Viewport)
    {
        SetViewport(Viewport.TopLeftX,
                    Viewport.TopLeftY,
                    Viewport.Width,
                    Viewport.Height,
                    Viewport.MinDepth,
                    Viewport.MaxDepth);
    }

    void D3D12GraphicsContext::SetViewport(FLOAT TopLeftX, FLOAT TopLeftY, FLOAT Width, FLOAT Height, FLOAT MinDepth, FLOAT MaxDepth)
    {
        Cache.Graphics.NumViewports = 1;
        Cache.Graphics.Viewports[0] = CD3DX12_VIEWPORT(TopLeftX, TopLeftY, Width, Height, MinDepth, MaxDepth);
        m_CommandListHandle->RSSetViewports(Cache.Graphics.NumViewports, Cache.Graphics.Viewports);
    }

    void D3D12GraphicsContext::SetViewports(std::vector<RHIViewport> Viewports)
    {
        Cache.Graphics.NumViewports = static_cast<uint32_t>(Viewports.size());
        uint32_t ViewportIndex          = 0;
        for (auto& Viewport : Viewports)
        {
            Cache.Graphics.Viewports[ViewportIndex++] = CD3DX12_VIEWPORT(Viewport.TopLeftX,
                                                                         Viewport.TopLeftY,
                                                                         Viewport.Width,
                                                                         Viewport.Height,
                                                                         Viewport.MinDepth,
                                                                         Viewport.MaxDepth);
        }
        m_CommandListHandle->RSSetViewports(Cache.Graphics.NumViewports, Cache.Graphics.Viewports);
    }

    void D3D12GraphicsContext::SetScissorRect(const RHIRect& ScissorRect)
    {
        SetScissorRect(ScissorRect.Left, ScissorRect.Top, ScissorRect.Right, ScissorRect.Bottom);
    }

    void D3D12GraphicsContext::SetScissorRect(uint32_t Left, uint32_t Top, uint32_t Right, uint32_t Bottom)
    {
        Cache.Graphics.NumScissorRects = 1;
        Cache.Graphics.ScissorRects[0] = CD3DX12_RECT(Left, Top, Right, Bottom);
        m_CommandListHandle->RSSetScissorRects(Cache.Graphics.NumScissorRects, Cache.Graphics.ScissorRects);
    }

    void D3D12GraphicsContext::SetScissorRects(std::vector<RHIRect> ScissorRects)
    {
        Cache.Graphics.NumScissorRects = static_cast<uint32_t>(ScissorRects.size());
        uint32_t ScissorRectIndex          = 0;
        for (auto& ScissorRect : ScissorRects)
        {
            Cache.Graphics.ScissorRects[ScissorRectIndex++] =
                CD3DX12_RECT(ScissorRect.Left, ScissorRect.Top, ScissorRect.Right, ScissorRect.Bottom);
        }
        m_CommandListHandle->RSSetScissorRects(Cache.Graphics.NumScissorRects, Cache.Graphics.ScissorRects);
    }

    void D3D12GraphicsContext::SetViewportAndScissorRect(const RHIViewport& vp, const RHIRect& rect)
    {
        SetViewport(vp);
        SetScissorRect(rect);
    }

    void D3D12GraphicsContext::SetViewportAndScissorRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
    {
        SetViewport((float)x, (float)y, (float)w, (float)h);
        SetScissorRect(x, y, x + w, y + h);
    }

    void D3D12GraphicsContext::SetStencilRef(uint32_t StencilRef) { m_CommandListHandle->OMSetStencilRef(StencilRef); }

    void D3D12GraphicsContext::SetBlendFactor(MoYu::Color BlendFactor)
    {
        m_CommandListHandle->OMSetBlendFactor((float*)&BlendFactor);
    }

    void D3D12GraphicsContext::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY Topology)
    {
        m_CommandListHandle->IASetPrimitiveTopology(Topology);
    }

    void D3D12GraphicsContext::SetConstantArray(uint32_t RootIndex, uint32_t Offset, uint32_t NumConstants, const void* pConstants)
    {
        m_CommandListHandle->SetGraphicsRoot32BitConstants(RootIndex, NumConstants, pConstants, Offset);
    }

    void D3D12GraphicsContext::SetConstantArray(uint32_t RootIndex, uint32_t NumConstants, const void* pConstants)
    {
        m_CommandListHandle->SetGraphicsRoot32BitConstants(RootIndex, NumConstants, pConstants, 0);
    }

    void D3D12GraphicsContext::SetConstant(uint32_t RootEntry, uint32_t Offset, DWParam Val)
    {
        m_CommandListHandle->SetGraphicsRoot32BitConstant(RootEntry, Val.Uint, Offset);
    }

    void D3D12GraphicsContext::SetConstants(uint32_t RootIndex, DWParam X)
    {
        m_CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
    }

    void D3D12GraphicsContext::SetConstants(uint32_t RootIndex, DWParam X, DWParam Y)
    {
        m_CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
        m_CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
    }

    void D3D12GraphicsContext::SetConstants(uint32_t RootIndex, DWParam X, DWParam Y, DWParam Z)
    {
        m_CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
        m_CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
        m_CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, Z.Uint, 2);
    }

    void D3D12GraphicsContext::SetConstants(uint32_t RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W)
    {
        m_CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
        m_CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
        m_CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, Z.Uint, 2);
        m_CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, W.Uint, 3);
    }

    void D3D12GraphicsContext::SetConstantBuffer(uint32_t RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV)
    {
        m_CommandListHandle->SetGraphicsRootConstantBufferView(RootIndex, CBV);
    }

    void D3D12GraphicsContext::SetDynamicConstantBufferView(uint32_t RootIndex, UINT64 BufferSize, const void* BufferData)
    {
        ASSERT(BufferData != nullptr && MoYu::IsAligned(BufferData, 16));
        GraphicsResource cb = m_GraphicsMemory->Allocate(BufferSize);
        // SIMDMemCopy(cb.DataPtr, BufferData, Math::AlignUp(BufferSize, 16) >> 4);
        memcpy(cb.Memory(), BufferData, BufferSize);
        m_CommandListHandle->SetGraphicsRootConstantBufferView(RootIndex, cb.GpuAddress());
    }

    void D3D12GraphicsContext::SetBufferSRV(uint32_t RootIndex, D3D12Buffer* BufferSRV, UINT64 Offset)
    {
        //ASSERT((BufferSRV->GetResourceState().GetSubresourceState(0) &
        //        (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)) != 0);
        ASSERT((m_CommandListHandle.GetAllTrackedResourceState(BufferSRV).GetSubresourceState(0) & 
            (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)) != 0);
        
        m_CommandListHandle->SetGraphicsRootShaderResourceView(RootIndex, BufferSRV->GetGpuVirtualAddress(0) + Offset);
    }

    void D3D12GraphicsContext::SetBufferUAV(uint32_t RootIndex, D3D12Buffer* BufferUAV, UINT64 Offset)
    {
        //ASSERT((BufferUAV->GetResourceState().GetSubresourceState(0) & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0);
        ASSERT((m_CommandListHandle.GetAllTrackedResourceState(BufferUAV).GetSubresourceState(0) & (D3D12_RESOURCE_STATE_UNORDERED_ACCESS)) != 0);
        m_CommandListHandle->SetGraphicsRootUnorderedAccessView(RootIndex, BufferUAV->GetGpuVirtualAddress(0) + Offset);
    }

    void D3D12GraphicsContext::SetDescriptorTable(uint32_t RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle)
    {
        m_CommandListHandle->SetGraphicsRootDescriptorTable(RootIndex, FirstHandle);
    }

    void D3D12GraphicsContext::SetDynamicDescriptor(uint32_t RootIndex, uint32_t Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
    {
        SetDynamicDescriptors(RootIndex, Offset, 1, &Handle);
    }

    void D3D12GraphicsContext::SetDynamicDescriptors(uint32_t RootIndex, uint32_t Offset, uint32_t Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
    {
        m_pDynamicViewDescriptorHeap->SetGraphicsDescriptorHandles(RootIndex, Offset, Count, Handles);
    }

    void D3D12GraphicsContext::SetDynamicSampler(uint32_t RootIndex, uint32_t Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
    {
        SetDynamicSamplers(RootIndex, Offset, 1, &Handle);
    }

    void D3D12GraphicsContext::SetDynamicSamplers(uint32_t RootIndex, uint32_t Offset, uint32_t Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
    {
        m_pDynamicSamplerDescriptorHeap->SetComputeDescriptorHandles(RootIndex, Offset, Count, Handles);
    }

    void D3D12GraphicsContext::SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& IBView)
    {
        m_CommandListHandle->IASetIndexBuffer(&IBView);
    }

    void D3D12GraphicsContext::SetVertexBuffer(uint32_t Slot, const D3D12_VERTEX_BUFFER_VIEW& VBView)
    {
        SetVertexBuffers(Slot, 1, &VBView);
    }

    void D3D12GraphicsContext::SetVertexBuffers(uint32_t StartSlot, uint32_t Count, const D3D12_VERTEX_BUFFER_VIEW VBViews[])
    {
        m_CommandListHandle->IASetVertexBuffers(StartSlot, Count, VBViews);
    }

    void D3D12GraphicsContext::SetDynamicVB(uint32_t Slot, UINT64 NumVertices, UINT64 VertexStride, const void* VertexData)
    {
        ASSERT(VertexData != nullptr && MoYu::IsAligned(VertexData, 16));

        size_t BufferSize = MoYu::AlignUp(NumVertices * VertexStride, 16);
        GraphicsResource vb = m_GraphicsMemory->Allocate(BufferSize);

        SIMDMemCopy(vb.Memory(), VertexData, BufferSize >> 4);

        D3D12_VERTEX_BUFFER_VIEW VBView;
        VBView.BufferLocation = vb.GpuAddress();
        VBView.SizeInBytes    = (uint32_t)BufferSize;
        VBView.StrideInBytes  = (uint32_t)VertexStride;

        m_CommandListHandle->IASetVertexBuffers(Slot, 1, &VBView);
    }

    void D3D12GraphicsContext::SetDynamicIB(UINT64 IndexCount, const UINT16* IndexData)
    {
        ASSERT(IndexData != nullptr && MoYu::IsAligned(IndexData, 16));

        size_t BufferSize = MoYu::AlignUp(IndexCount * sizeof(uint16_t), 16);
        GraphicsResource ib = m_GraphicsMemory->Allocate(BufferSize);

        SIMDMemCopy(ib.Memory(), IndexData, BufferSize >> 4);

        D3D12_INDEX_BUFFER_VIEW IBView;
        IBView.BufferLocation = ib.GpuAddress();
        IBView.SizeInBytes    = (uint32_t)(IndexCount * sizeof(uint16_t));
        IBView.Format         = DXGI_FORMAT_R16_UINT;

        m_CommandListHandle->IASetIndexBuffer(&IBView);
    }

    void D3D12GraphicsContext::SetDynamicSRV(uint32_t RootIndex, UINT64 BufferSize, const void* BufferData)
    {
        ASSERT(BufferData != nullptr && MoYu::IsAligned(BufferData, 16));
        GraphicsResource cb = m_GraphicsMemory->Allocate(BufferSize);
        SIMDMemCopy(cb.Memory(), BufferData, MoYu::AlignUp(BufferSize, 16) >> 4);
        m_CommandListHandle->SetGraphicsRootShaderResourceView(RootIndex, cb.GpuAddress());
    }

    void D3D12GraphicsContext::Draw(uint32_t VertexCount, uint32_t VertexStartOffset)
    {
        DrawInstanced(VertexCount, 1, VertexStartOffset, 0);
    }

    void D3D12GraphicsContext::DrawIndexed(uint32_t IndexCount, uint32_t StartIndexLocation, INT BaseVertexLocation)
    {
        DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
    }

    void D3D12GraphicsContext::DrawInstanced(uint32_t VertexCountPerInstance,
                                             uint32_t InstanceCount,
                                             uint32_t StartVertexLocation,
                                             uint32_t StartInstanceLocation)
    {
        FlushResourceBarriers();
        m_pDynamicViewDescriptorHeap->CommitGraphicsRootDescriptorTables(m_CommandListHandle.GetGraphicsCommandList());
        m_pDynamicSamplerDescriptorHeap->CommitGraphicsRootDescriptorTables(m_CommandListHandle.GetGraphicsCommandList());
        m_CommandListHandle->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
    }

    void D3D12GraphicsContext::DrawIndexedInstanced(uint32_t IndexCountPerInstance,
                                                    uint32_t InstanceCount,
                                                    uint32_t StartIndexLocation,
                                                    INT  BaseVertexLocation,
                                                    uint32_t StartInstanceLocation)
    {
        FlushResourceBarriers();
        m_pDynamicViewDescriptorHeap->CommitGraphicsRootDescriptorTables(m_CommandListHandle.GetGraphicsCommandList());
        m_pDynamicSamplerDescriptorHeap->CommitGraphicsRootDescriptorTables(m_CommandListHandle.GetGraphicsCommandList());
        m_CommandListHandle->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    }

    void D3D12GraphicsContext::DrawIndirect(D3D12Buffer* ArgumentBuffer, UINT64 ArgumentBufferOffset)
    {
        ExecuteIndirect(RHI::pDrawIndirectCommandSignature, ArgumentBuffer, ArgumentBufferOffset);
    }

    void D3D12GraphicsContext::ExecuteIndirect(D3D12CommandSignature* CommandSig,
                                               D3D12Buffer*           ArgumentBuffer,
                                               UINT64                 ArgumentStartOffset,
                                               UINT32                 MaxCommands,
                                               D3D12Buffer*           CommandCounterBuffer,
                                               UINT64                 CounterOffset)
    {
        FlushResourceBarriers();
        m_pDynamicViewDescriptorHeap->CommitGraphicsRootDescriptorTables(m_CommandListHandle.GetGraphicsCommandList());
        m_pDynamicSamplerDescriptorHeap->CommitGraphicsRootDescriptorTables(m_CommandListHandle.GetGraphicsCommandList());
        m_CommandListHandle->ExecuteIndirect(CommandSig->GetApiHandle(),
                                             MaxCommands,
                                             ArgumentBuffer->GetResource(),
                                             ArgumentStartOffset,
                                             CommandCounterBuffer == nullptr ? nullptr : CommandCounterBuffer->GetResource(),
                                             CounterOffset);
    }

    // ================================Compute=====================================

    void D3D12ComputeContext::ClearUAV(RHI::D3D12Buffer* Target)
    {
        FlushResourceBarriers();

        std::shared_ptr<D3D12UnorderedAccessView> uav = Target->GetDefaultUAV();
        std::shared_ptr<D3D12UnorderedAccessView> cpu_uav = Target->GetDefaultUAV(TRUE);
        const uint32_t ClearColor[4] = {0, 0, 0, 0};
        m_CommandListHandle->ClearUnorderedAccessViewUint(
            uav->GetGpuHandle(), cpu_uav->GetCpuHandle(), Target->GetResource(), ClearColor, 0, nullptr);
    }

    void D3D12ComputeContext::ClearUAV(RHI::D3D12Texture* Target)
    {
        FlushResourceBarriers();

        std::shared_ptr<D3D12UnorderedAccessView> uav = Target->GetDefaultUAV();
        std::shared_ptr<D3D12UnorderedAccessView> cpu_uav = Target->GetDefaultUAV(TRUE);
        CD3DX12_RECT clearRect(0, 0, (LONG)Target->GetWidth(), (LONG)Target->GetHeight());
        D3D12_CLEAR_VALUE clearVal = Target->GetClearValue();
        m_CommandListHandle->ClearUnorderedAccessViewFloat(
            uav->GetGpuHandle(), cpu_uav->GetCpuHandle(), Target->GetResource(), clearVal.Color, 1, &clearRect);
    }

	void D3D12ComputeContext::SetRootSignature(D3D12RootSignature* RootSignature)
    {
        if (Cache.RootSignature == RootSignature)
            return;

        m_CommandListHandle->SetComputeRootSignature(RootSignature->GetApiHandle());

        m_pDynamicViewDescriptorHeap->ParseComputeRootSignature(RootSignature);
        m_pDynamicSamplerDescriptorHeap->ParseComputeRootSignature(RootSignature);

        /*
        if (Cache.RootSignature != RootSignature)
        {
            Cache.RootSignature = RootSignature;

            m_CommandListHandle->SetComputeRootSignature(RootSignature->GetApiHandle());

            SetDynamicResourceDescriptorTables<RHI_PIPELINE_STATE_TYPE::Compute>(RootSignature);
        }
        */
    }

    void D3D12ComputeContext::SetConstantArray(uint32_t RootIndex, uint32_t NumConstants, const void* pConstants)
    {
        m_CommandListHandle->SetComputeRoot32BitConstants(RootIndex, NumConstants, pConstants, 0);
    }

    void D3D12ComputeContext::SetConstant(uint32_t RootEntry, uint32_t Offset, DWParam Val)
    {
        m_CommandListHandle->SetComputeRoot32BitConstant(RootEntry, Val.Uint, Offset);
    }

    void D3D12ComputeContext::SetConstants(uint32_t RootIndex, DWParam X)
    {
        m_CommandListHandle->SetComputeRoot32BitConstant(RootIndex, X.Uint, 0);
    }

    void D3D12ComputeContext::SetConstants(uint32_t RootIndex, DWParam X, DWParam Y)
    {
        m_CommandListHandle->SetComputeRoot32BitConstant(RootIndex, X.Uint, 0);
        m_CommandListHandle->SetComputeRoot32BitConstant(RootIndex, Y.Uint, 1);
    }

    void D3D12ComputeContext::SetConstants(uint32_t RootIndex, DWParam X, DWParam Y, DWParam Z)
    {
        m_CommandListHandle->SetComputeRoot32BitConstant(RootIndex, X.Uint, 0);
        m_CommandListHandle->SetComputeRoot32BitConstant(RootIndex, Y.Uint, 1);
        m_CommandListHandle->SetComputeRoot32BitConstant(RootIndex, Z.Uint, 2);
    }

    void D3D12ComputeContext::SetConstants(uint32_t RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W)
    {
        m_CommandListHandle->SetComputeRoot32BitConstant(RootIndex, X.Uint, 0);
        m_CommandListHandle->SetComputeRoot32BitConstant(RootIndex, Y.Uint, 1);
        m_CommandListHandle->SetComputeRoot32BitConstant(RootIndex, Z.Uint, 2);
        m_CommandListHandle->SetComputeRoot32BitConstant(RootIndex, W.Uint, 3);
    }

    void D3D12ComputeContext::SetConstantBuffer(uint32_t RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV)
    {
        m_CommandListHandle->SetComputeRootConstantBufferView(RootIndex, CBV);
    }

    void D3D12ComputeContext::SetDynamicConstantBufferView(uint32_t RootIndex, UINT64 BufferSize, const void* BufferData)
    {
        ASSERT(BufferData != nullptr && MoYu::IsAligned(BufferData, 16));
        GraphicsResource cb = m_GraphicsMemory->Allocate(BufferSize, 256);
        // SIMDMemCopy(cb.DataPtr, BufferData, Math::AlignUp(BufferSize, 16) >> 4);
        memcpy(cb.Memory(), BufferData, BufferSize);
        m_CommandListHandle->SetComputeRootConstantBufferView(RootIndex, cb.GpuAddress());
    }

    void D3D12ComputeContext::SetDynamicSRV(uint32_t RootIndex, UINT64 BufferSize, const void* BufferData)
    {
        ASSERT(BufferData != nullptr && MoYu::IsAligned(BufferData, 16));
        GraphicsResource cb = m_GraphicsMemory->Allocate(BufferSize, 256);
        SIMDMemCopy(cb.Memory(), BufferData, MoYu::AlignUp(BufferSize, 16) >> 4);
        m_CommandListHandle->SetComputeRootShaderResourceView(RootIndex, cb.GpuAddress());
    }

    void D3D12ComputeContext::SetBufferSRV(uint32_t RootIndex, D3D12Buffer* BufferSRV, UINT64 Offset)
    {
        //ASSERT((BufferSRV->GetResourceState().GetSubresourceState(0) & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) != 0);
        ASSERT((m_CommandListHandle.GetAllTrackedResourceState(BufferSRV).GetSubresourceState(0) & (D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)) != 0);
        m_CommandListHandle->SetComputeRootShaderResourceView(RootIndex, BufferSRV->GetGpuVirtualAddress(0) + Offset);
    }

    void D3D12ComputeContext::SetBufferUAV(uint32_t RootIndex, D3D12Buffer* BufferUAV, UINT64 Offset)
    {
        //ASSERT((BufferUAV->GetResourceState().GetSubresourceState(0) & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0);
        ASSERT((m_CommandListHandle.GetAllTrackedResourceState(BufferUAV).GetSubresourceState(0) & (D3D12_RESOURCE_STATE_UNORDERED_ACCESS)) != 0);
        m_CommandListHandle->SetComputeRootUnorderedAccessView(RootIndex, BufferUAV->GetGpuVirtualAddress(0) + Offset);
    }

    void D3D12ComputeContext::SetDescriptorTable(uint32_t RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle)
    {
        m_CommandListHandle->SetComputeRootDescriptorTable(RootIndex, FirstHandle);
    }

    void D3D12ComputeContext::SetDynamicDescriptor(uint32_t RootIndex, uint32_t Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
    {
        SetDynamicDescriptors(RootIndex, Offset, 1, &Handle);
    }

    void D3D12ComputeContext::SetDynamicDescriptors(uint32_t RootIndex, uint32_t Offset, uint32_t Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
    {
        m_pDynamicViewDescriptorHeap->SetComputeDescriptorHandles(RootIndex, Offset, Count, Handles);
    }

    void D3D12ComputeContext::SetDynamicSampler(uint32_t RootIndex, uint32_t Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
    {
        SetDynamicSamplers(RootIndex, Offset, 1, &Handle);
    }

    void D3D12ComputeContext::SetDynamicSamplers(uint32_t RootIndex, uint32_t Offset, uint32_t Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
    {
        m_pDynamicSamplerDescriptorHeap->SetComputeDescriptorHandles(RootIndex, Offset, Count, Handles);
    }

    void D3D12ComputeContext::Dispatch(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ)
    {
        m_CommandListHandle.FlushResourceBarriers();
        m_pDynamicViewDescriptorHeap->CommitComputeRootDescriptorTables(m_CommandListHandle.GetGraphicsCommandList());
        m_pDynamicSamplerDescriptorHeap->CommitComputeRootDescriptorTables(m_CommandListHandle.GetGraphicsCommandList());
        m_CommandListHandle->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
    }

    void D3D12ComputeContext::Dispatch1D(UINT64 ThreadCountX, UINT64 GroupSizeX)
    {
        m_CommandListHandle.FlushResourceBarriers();
        uint32_t ThreadGroupCountX = RoundUpAndDivide(ThreadCountX, GroupSizeX);
        Dispatch(ThreadGroupCountX, 1, 1);
    }

    void D3D12ComputeContext::Dispatch2D(UINT64 ThreadCountX, UINT64 ThreadCountY, UINT64 GroupSizeX, UINT64 GroupSizeY)
    {
        m_CommandListHandle.FlushResourceBarriers();
        uint32_t ThreadGroupCountX = RoundUpAndDivide(ThreadCountX, GroupSizeX);
        uint32_t ThreadGroupCountY = RoundUpAndDivide(ThreadCountY, GroupSizeY);
        Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
    }

    void D3D12ComputeContext::Dispatch3D(UINT64 ThreadCountX, UINT64 ThreadCountY, UINT64 ThreadCountZ, UINT64 GroupSizeX, UINT64 GroupSizeY, UINT64 GroupSizeZ)
    {
        m_CommandListHandle.FlushResourceBarriers();
        uint32_t ThreadGroupCountX = RoundUpAndDivide(ThreadCountX, GroupSizeX);
        uint32_t ThreadGroupCountY = RoundUpAndDivide(ThreadCountY, GroupSizeY);
        uint32_t ThreadGroupCountZ = RoundUpAndDivide(ThreadCountZ, GroupSizeZ);
        Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
    }

    void D3D12ComputeContext::DispatchIndirect(D3D12Buffer* ArgumentBuffer, UINT64 ArgumentBufferOffset)
    {
        ExecuteIndirect(RHI::pDispatchIndirectCommandSignature, ArgumentBuffer, ArgumentBufferOffset);
    }

    void D3D12ComputeContext::ExecuteIndirect(D3D12CommandSignature* CommandSig,
                                              D3D12Buffer*           ArgumentBuffer,
                                              UINT64                 ArgumentStartOffset,
                                              UINT32                 MaxCommands,
                                              D3D12Buffer*           CommandCounterBuffer,
                                              UINT64                 CounterOffset)
    {
        FlushResourceBarriers();
        m_pDynamicViewDescriptorHeap->CommitComputeRootDescriptorTables(m_CommandListHandle.GetGraphicsCommandList());
        m_pDynamicSamplerDescriptorHeap->CommitComputeRootDescriptorTables(m_CommandListHandle.GetGraphicsCommandList());
        m_CommandListHandle->ExecuteIndirect(CommandSig->GetApiHandle(),
                                             MaxCommands,
                                             ArgumentBuffer->GetResource(),
                                             ArgumentStartOffset,
                                             CommandCounterBuffer == nullptr ? nullptr : CommandCounterBuffer->GetResource(),
                                             CounterOffset);
    }

    D3D12ScopedEventObject::D3D12ScopedEventObject(D3D12CommandContext* CommandContext, std::string_view Name) :
        ProfileBlock(CommandContext->GetParentLinkedDevice()->GetProfiler(),
                     CommandContext->GetCommandQueue(),
                     CommandContext->GetGraphicsCommandList(),
                     Name)
#ifdef _DEBUG
        ,
        PixEvent(CommandContext->GetGraphicsCommandList(), 0, Name.data())
#endif
    {
        // Copy queue profiling currently not supported
        ASSERT(CommandContext->GetCommandQueue()->GetType() != D3D12_COMMAND_LIST_TYPE_COPY);
    }

    void D3D12CommandContext::InitializeTexture(D3D12LinkedDevice* Parent, D3D12Texture* Dest, std::vector<D3D12_SUBRESOURCE_DATA> Subresources)
    {
        D3D12CommandContext::InitializeTexture(Parent, Dest, 0, Subresources);
    }

    void D3D12CommandContext::InitializeTexture(D3D12LinkedDevice* Parent, D3D12Texture* Dest, uint32_t FirstSubresource, std::vector<D3D12_SUBRESOURCE_DATA> Subresources)
    {
        /*
        Parent->BeginResourceUpload();
        Parent->Upload(Subresources, FirstSubresource, Dest->GetResource());
        Parent->EndResourceUpload(true);
        */

        if (Subresources.empty())
            return;

        uint32_t   NumSubresources  = Subresources.size();
        UINT64 uploadBufferSize = GetRequiredIntermediateSize(Dest->GetResource(), FirstSubresource, NumSubresources);

        D3D12CommandContext* InitContext = Parent->GetCopyContext2();
        InitContext->Open();

        D3D12_RESOURCE_STATES originalState = Dest->GetResourceState().GetSubresourceState(FirstSubresource);

        InitContext->TransitionBarrier(Dest, D3D12_RESOURCE_STATE_COPY_DEST, FirstSubresource, true);

        // copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default
        // texture
        RHI::GraphicsResource mem = InitContext->ReserveUploadMemory(uploadBufferSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
        UpdateSubresources(InitContext->GetGraphicsCommandList(),
                           Dest->GetResource(),
                           mem.Resource(),
                           mem.ResourceOffset(),
                           FirstSubresource,
                           NumSubresources,
                           Subresources.data());
        InitContext->TransitionBarrier(Dest, originalState, FirstSubresource, true);

        InitContext->Close();

        // Execute the command list and wait for it to finish so we can release the upload buffer
        InitContext->Execute(true);

        /*
        RHI::ResourceUploadBatch* pUploadBatch    = Parent->GetResourceUploadBatch();
        RHI::GraphicsMemory*      pGraphicsMemory = Parent->GetGraphicsMemory();

        D3D12CommandContext* pUploadContext = Parent->GetCopyContext2();

        pUploadBatch->Begin();

        D3D12_RESOURCE_STATES originalState = Dest->GetResourceState().GetSubresourceState(0);

        pUploadBatch->Transition(Dest->GetResource(), originalState, D3D12_RESOURCE_STATE_COPY_DEST);
        pUploadBatch->Upload(Dest->GetResource(), FirstSubresource, Subresources.data(), Subresources.size());
        pUploadBatch->Transition(Dest->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, originalState);

        std::future<void> uploadResourcesFinished = pUploadBatch->End(pUploadContext->GetCommandQueue()->GetCommandQueue());
        uploadResourcesFinished.wait();
        */
    }

    void D3D12CommandContext::InitializeBuffer(D3D12LinkedDevice* Parent, D3D12Buffer* Dest, const void* Data, UINT64 NumBytes, UINT64 DestOffset)
    {
        if (Data == nullptr)
            return;

        D3D12CommandContext* InitContext = Parent->GetCopyContext2();
        InitContext->Open();

        RHI::GraphicsResource mem = InitContext->ReserveUploadMemory(NumBytes);
        SIMDMemCopy(mem.Memory(), Data, MoYu::DivideByMultiple(NumBytes, 16));

        D3D12_RESOURCE_STATES originalState = Dest->GetResourceState().GetSubresourceState(0);

        // copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default
        // texture
        InitContext->TransitionBarrier(
            Dest, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
        (*InitContext)->CopyBufferRegion(Dest->GetResource(), DestOffset, mem.Resource(), 0, NumBytes);
        InitContext->TransitionBarrier(Dest, originalState, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);

        InitContext->Close();

        // Execute the command list and wait for it to finish so we can release the upload buffer
        InitContext->Execute(true);

        /*
        RHI::ResourceUploadBatch* pUploadBatch = Parent->GetResourceUploadBatch();
        RHI::GraphicsMemory* pGraphicsMemory = Parent->GetGraphicsMemory();

        D3D12CommandContext* pUploadContext = Parent->GetCopyContext2();

        pUploadBatch->Begin();

        D3D12_RESOURCE_STATES originalState = Dest->GetResourceState().GetSubresourceState(0);

        SharedGraphicsResource mem = pGraphicsMemory->Allocate(NumBytes, 16);
        SIMDMemCopy(mem.Memory(), Data, MoYu::DivideByMultiple(NumBytes, 16));

        pUploadBatch->Transition(Dest->GetResource(), originalState, D3D12_RESOURCE_STATE_COPY_DEST);
        pUploadBatch->Upload(Dest->GetResource(), DestOffset, mem);
        pUploadBatch->Transition(Dest->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, originalState);

        std::future<void> uploadResourcesFinished = pUploadBatch->End(pUploadContext->GetCommandQueue()->GetCommandQueue());
        uploadResourcesFinished.wait();
        */
    }

    void D3D12CommandContext::InitializeTextureArraySlice(D3D12LinkedDevice* Parent, D3D12Texture* Dest, uint32_t SliceIndex, D3D12Texture* Src)
    {
        D3D12CommandContext* InitContext = Parent->GetCopyContext2();
        InitContext->Open();

        InitContext->TransitionBarrier(Dest, D3D12_RESOURCE_STATE_COPY_DEST, true);

        const CD3DX12_RESOURCE_DESC& DestDesc = Dest->GetDesc();
        const CD3DX12_RESOURCE_DESC& SrcDesc  = Src->GetDesc();

        ASSERT(SliceIndex < DestDesc.DepthOrArraySize && SrcDesc.DepthOrArraySize == 1 &&
               DestDesc.Width == SrcDesc.Width && DestDesc.Height == SrcDesc.Height &&
               DestDesc.MipLevels <= SrcDesc.MipLevels);

        uint32_t SubResourceIndex = SliceIndex * DestDesc.MipLevels;

        for (uint32_t i = 0; i < DestDesc.MipLevels; ++i)
        {
            D3D12_TEXTURE_COPY_LOCATION destCopyLocation = {
                Dest->GetResource(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, SubResourceIndex + i};

            D3D12_TEXTURE_COPY_LOCATION srcCopyLocation = {
                Src->GetResource(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, i};

            (*InitContext)->CopyTextureRegion(&destCopyLocation, 0, 0, 0, &srcCopyLocation, nullptr);
        }

        InitContext->TransitionBarrier(Dest, D3D12_RESOURCE_STATE_GENERIC_READ);

        InitContext->Close();

        InitContext->Execute(true);
    }

















}
