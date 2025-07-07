#include "d3d12_commandList.h"
#include "d3d12_linkedDevice.h"
#include "d3d12_resource.h"
#include "runtime/core/math/moyu_math2.h"
#include <codecvt>

namespace RHI
{
    std::vector<PendingResourceBarrier>& D3D12ResourceStateTracker::GetPendingResourceBarriers()
    {
        return PendingResourceBarriers;
    }

    void D3D12ResourceStateTracker::ClearPendingResourceBarrier()
    {
        PendingResourceBarriers.clear();
    }

	CResourceState& D3D12ResourceStateTracker::GetResourceState(D3D12Resource* Resource)
    {
        CResourceState& ResourceState = ResourceStates[Resource->GetResource()];
        // If ResourceState was just created, its state is uninitialized
        if (ResourceState.IsUninitialized())
        {
            ResourceState = CResourceState(Resource->GetNumSubresources(), D3D12_RESOURCE_STATE_UNKNOWN);
        }
        return ResourceState;
    }

    void D3D12ResourceStateTracker::SetResourceState(D3D12Resource* Resource, const CResourceState ResourceState)
    {
        ResourceStates[Resource->GetResource()] = ResourceState;
    }

    std::vector<CachedResourceBarrier>& D3D12ResourceStateTracker::GetCachedResourceBarriers()
    {
        return CachedResourceBarrierVector;
    }

    void D3D12ResourceStateTracker::ClearCachedResourceBarriers()
    {
        CachedResourceBarrierVector.clear();
    }

    void D3D12ResourceStateTracker::AddCachedResourceBarrier(const CachedResourceBarrier& CachedResourceBarrier)
    {
        CachedResourceBarrierVector.push_back(CachedResourceBarrier);
    }
    
    void D3D12ResourceStateTracker::Reset()
    {
        ResourceStates.clear();
        PendingResourceBarriers.clear();
    }

    void D3D12ResourceStateTracker::AddPendingBarrier(const PendingResourceBarrier& PendingResourceBarrier)
    {
        PendingResourceBarriers.push_back(PendingResourceBarrier);
    }

    D3D12CommandListHandle::D3D12CommandListHandle(D3D12LinkedDevice* Parent, D3D12_COMMAND_LIST_TYPE Type) :
        D3D12LinkedDeviceChild(Parent), Type(Type), GraphicsCommandList([&] {
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GraphicsCommandList;
            VERIFY_D3D12_API(Parent->GetDevice5()->CreateCommandList1(
                Parent->GetNodeMask(), Type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&GraphicsCommandList)));
            return GraphicsCommandList;
        }())
    {
        GraphicsCommandList->QueryInterface(IID_PPV_ARGS(&GraphicsCommandList4));
        GraphicsCommandList->QueryInterface(IID_PPV_ARGS(&GraphicsCommandList6));
#ifdef MOYU_RHI_D3D12_DEBUG_RESOURCE_STATES
        GraphicsCommandList->QueryInterface(IID_PPV_ARGS(&DebugCommandList));
#endif
        CommandListState = D3D12CommandListState::Closed;
    }

	bool D3D12CommandListHandle::Open(ID3D12CommandAllocator* CommandAllocator)
    {
        // skip reset if current commandlist is recording now
        if (CommandListState == D3D12CommandListState::Recording)
            return false;

        VERIFY_D3D12_API(GraphicsCommandList->Reset(CommandAllocator, nullptr));

        // Reset resource state tracking and resource barriers
        ResourceStateTracker.Reset();
        NumResourceBarriers = 0;

        // start commandList state
        CommandListState = D3D12CommandListState::Recording;

        return true;
    }

    void D3D12CommandListHandle::Close()
    {
        CommandListState = D3D12CommandListState::Pending;

        FlushResourceBarriers();
        VERIFY_D3D12_API(GraphicsCommandList->Close());
    }

    const CResourceState& D3D12CommandListHandle::GetAllTrackedResourceState(D3D12Resource* Resource)
    {
        const CResourceState& ResourceState = ResourceStateTracker.GetResourceState(Resource);
        return ResourceState;
    }

    D3D12_RESOURCE_STATES D3D12CommandListHandle::GetResourceStateTracked(D3D12Resource* Resource, UINT Subresource)
    {
        CResourceState& ResourceState = ResourceStateTracker.GetResourceState(Resource);
        return ResourceState.GetSubresourceState(Subresource);
    }

    void D3D12CommandListHandle::TransitionBarrier(D3D12Resource*        Resource,
                                                   D3D12_RESOURCE_STATES State,
                                                   UINT Subresource /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/)
    {
        CResourceState& ResourceState = ResourceStateTracker.GetResourceState(Resource);
        // First use on the command list
        if (ResourceState.IsUnknown(Subresource))
        {
            ResourceStateTracker.AddPendingBarrier(PendingResourceBarrier {Resource, State, Subresource});
        }
        // Known state within the command list
        else
        {
            // If we are applying transition to all subresources and we are in different tracking mode
            // transition each subresource individually
            if (Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES && !ResourceState.IsUniform())
            {
                // First transition all of the subresources if they are different than the State
                UINT i = 0;
                for (D3D12_RESOURCE_STATES SubresourceState : ResourceState)
                {
                    if (SubresourceState != State)
                    {
#ifdef MOYU_RHI_D3D12_DEBUG_RESOURCE_STATES
                        using convert_type = std::codecvt_utf8<wchar_t>;
                        std::wstring_convert<convert_type, wchar_t> converter;
                        std::string ResourceName = converter.to_bytes(Resource->GetResourceName());
                        std::string SourceState = GetResourceStateNameFromState(SubresourceState);
                        std::string TargetState = GetResourceStateNameFromState(State);
                        std::string CachedStr = fmt::format("Resource {} Subresource {} State From {} to {}", ResourceName, i, SourceState, TargetState);
                        StateTransitionCacheList.push_back(CachedStr);
#endif
                        AddTransition(Resource, SubresourceState, State, i);
                    }

                    i++;
                }
            }
            // Apply barrier at subresource level
            else
            {
                D3D12_RESOURCE_STATES StateKnown = ResourceState.GetSubresourceState(Subresource);
                if (StateKnown != State)
                {
#ifdef MOYU_RHI_D3D12_DEBUG_RESOURCE_STATES
                    using convert_type = std::codecvt_utf8<wchar_t>;
                    std::wstring_convert<convert_type, wchar_t> converter;
                    std::string ResourceName = converter.to_bytes(Resource->GetResourceName());
                    std::string SourceState = GetResourceStateNameFromState(StateKnown);
                    std::string TargetState = GetResourceStateNameFromState(State);
                    std::string CachedStr = fmt::format("Resource {} Subresource {} State From {} to {}", ResourceName, Subresource, SourceState, TargetState);
                    StateTransitionCacheList.push_back(CachedStr);
#endif
                    AddTransition(Resource, StateKnown, State, Subresource);
                }
            }
        }

        // Update command list resource state
        ResourceState.SetSubresourceState(Subresource, State);
    }

    void D3D12CommandListHandle::AliasingBarrier(D3D12Resource* BeforeResource, D3D12Resource* AfterResource)
    {
        AddAliasing(BeforeResource, AfterResource);
    }

    void D3D12CommandListHandle::UAVBarrier(D3D12Resource* Resource)
    {
        AddUAV(Resource);
    }

    void D3D12CommandListHandle::FlushResourceBarriers()
    {
        std::vector<D3D12_RESOURCE_BARRIER> PendingResourceBarriersVector = ResolveResourceBarriers();
        
        int PendingBatchNum = glm::ceil(PendingResourceBarriersVector.size() / NumBatches);
        for (int i = 0; i < PendingBatchNum; i++)
        {
            int is = i * NumBatches;
            int ie = glm::fmin((i + 1) * NumBatches, PendingResourceBarriersVector.size());
            int t = 0;
            for (int j = is; j < ie; j++)
            {
                PendingResourceBarriers[t++] = PendingResourceBarriersVector[j];
            }
            NumPendingResourceBarriers = ie - is;
            GraphicsCommandList->ResourceBarrier(NumPendingResourceBarriers, PendingResourceBarriers);
        }
        ResourceStateTracker.ClearPendingResourceBarrier();

        std::vector<CachedResourceBarrier>& CachedResourceBarriers = ResourceStateTracker.GetCachedResourceBarriers();
        for (int i = 0; i < CachedResourceBarriers.size(); i++)
        {
            CachedResourceBarrier& CacheBarrier = CachedResourceBarriers[i];
            CResourceState& ResourceState = CacheBarrier.Resource->GetResourceState();
            ResourceState.SetSubresourceState(CacheBarrier.Subresource, CacheBarrier.State);
        }
        CachedResourceBarriers.clear();

        if (NumResourceBarriers > 0)
        {
#ifdef MOYU_RHI_D3D12_DEBUG_RESOURCE_STATES
            for (int i = 0; i < StateTransitionCacheList.size(); i++)
            {
                LOG_INFO(StateTransitionCacheList[i]);
            }
            StateTransitionCacheList.clear();
            
            LOG_INFO("=============== Resource Transition Flush ===============");
#endif

            GraphicsCommandList->ResourceBarrier(NumResourceBarriers, ResourceBarriers);
            NumResourceBarriers = 0;
        }
    }

    bool
    D3D12CommandListHandle::AssertResourceState(D3D12Resource* Resource, D3D12_RESOURCE_STATES State, UINT Subresource)
    {
#ifdef MOYU_RHI_D3D12_DEBUG_RESOURCE_STATES
        if (DebugCommandList)
        {
            return DebugCommandList->AssertResourceState(Resource->GetResource(), Subresource, State);
        }
#endif
        return true;
    }

    std::vector<D3D12_RESOURCE_BARRIER> D3D12CommandListHandle::ResolveResourceBarriers()
    {
        const auto& PendingResourceBarriers = ResourceStateTracker.GetPendingResourceBarriers();

        std::vector<D3D12_RESOURCE_BARRIER> ResourceBarriers;
        ResourceBarriers.reserve(PendingResourceBarriers.size());

        for (const auto& [Resource, State, Subresource] : PendingResourceBarriers)
        {
            CResourceState& ResourceState = Resource->GetResourceState();

            if ((Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) && !ResourceState.IsUniform())
            {
                int subresourceNumber = Resource->GetNumSubresources();
                CResourceState CacheState(subresourceNumber, State);
                for (int subIdx = 0; subIdx < subresourceNumber; subIdx++)
                {
                    D3D12_RESOURCE_STATES StateBefore = ResourceState.GetSubresourceState(subIdx);
                    D3D12_RESOURCE_STATES StateAfter  = State != D3D12_RESOURCE_STATE_UNKNOWN ? State : StateBefore;

                    if (StateBefore != StateAfter)
                    {
                        ResourceBarriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
                            Resource->GetResource(), StateBefore, StateAfter, subIdx));
#ifdef MOYU_RHI_D3D12_DEBUG_RESOURCE_STATES
                        using convert_type = std::codecvt_utf8<wchar_t>;
                        std::wstring_convert<convert_type, wchar_t> converter;
                        std::string ResourceName = converter.to_bytes(Resource->GetResourceName());
                        std::string SourceState = GetResourceStateNameFromState(StateBefore);
                        std::string TargetState = GetResourceStateNameFromState(StateAfter);
                        LOG_INFO("Resource {} Subresource {} State From {} to {}", ResourceName, subIdx, SourceState, TargetState);
#endif
                    }

                    // Get the command list resource state associate with this resource
                    D3D12_RESOURCE_STATES StateCommandList =
                        ResourceStateTracker.GetResourceState(Resource).GetSubresourceState(subIdx);
                    D3D12_RESOURCE_STATES StatePrevious =
                        StateCommandList != D3D12_RESOURCE_STATE_UNKNOWN ? StateCommandList : StateAfter;

                    // If global state is not same as commandlist space state, then change the global state
                    if (StateBefore != StatePrevious)
                    {
                        ResourceState.SetSubresourceState(subIdx, StatePrevious);
                    }
                }
                ResourceStateTracker.SetResourceState(Resource, CacheState);
            }
            else
            {
                D3D12_RESOURCE_STATES StateBefore = ResourceState.GetSubresourceState(Subresource);
                D3D12_RESOURCE_STATES StateAfter  = State != D3D12_RESOURCE_STATE_UNKNOWN ? State : StateBefore;
                
                if (StateBefore != StateAfter)
                {
                    ResourceBarriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
                        Resource->GetResource(), StateBefore, StateAfter, Subresource));
#ifdef MOYU_RHI_D3D12_DEBUG_RESOURCE_STATES
                    using convert_type = std::codecvt_utf8<wchar_t>;
                    std::wstring_convert<convert_type, wchar_t> converter;
                    std::string ResourceName = converter.to_bytes(Resource->GetResourceName());
                    std::string SourceState = GetResourceStateNameFromState(StateBefore);
                    std::string TargetState = GetResourceStateNameFromState(StateAfter);
                    LOG_INFO("Resource {} Subresource {} State From {} to {}", ResourceName, Subresource, SourceState, TargetState);
#endif
                }

                // Get the command list resource state associate with this resource
                D3D12_RESOURCE_STATES StateCommandList =
                    ResourceStateTracker.GetResourceState(Resource).GetSubresourceState(Subresource);
                D3D12_RESOURCE_STATES StatePrevious =
                    StateCommandList != D3D12_RESOURCE_STATE_UNKNOWN ? StateCommandList : StateAfter;

                // If global state is not same as commandlist space state, then change the global state
                if (StateBefore != StatePrevious)
                {
                    ResourceState.SetSubresourceState(Subresource, StatePrevious);
                }
            }
            CResourceState& CacheState = ResourceStateTracker.GetResourceState(Resource);
            CacheState.SetSubresourceState(Subresource, State);
            ResourceStateTracker.SetResourceState(Resource, CacheState);
        }

        return ResourceBarriers;
    }

    void D3D12CommandListHandle::Add(const D3D12_RESOURCE_BARRIER& ResourceBarrier)
    {
        assert(NumResourceBarriers < NumBatches);
        ResourceBarriers[NumResourceBarriers++] = ResourceBarrier;
        if (NumResourceBarriers == NumBatches)
        {
            FlushResourceBarriers();
        }
    }

    void D3D12CommandListHandle::AddTransition(D3D12Resource*        Resource,
                                               D3D12_RESOURCE_STATES StateBefore,
                                               D3D12_RESOURCE_STATES StateAfter,
                                               UINT Subresource /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/)
    {
        ResourceStateTracker.AddCachedResourceBarrier(CachedResourceBarrier{ Resource, StateAfter, Subresource });
        Add(CD3DX12_RESOURCE_BARRIER::Transition(Resource->GetResource(), StateBefore, StateAfter, Subresource));
    }

    void D3D12CommandListHandle::AddAliasing(D3D12Resource* BeforeResource, D3D12Resource* AfterResource)
    {
        Add(CD3DX12_RESOURCE_BARRIER::Aliasing(BeforeResource->GetResource(), AfterResource->GetResource()));
    }

    void D3D12CommandListHandle::AddUAV(D3D12Resource* Resource)
    {
        Add(CD3DX12_RESOURCE_BARRIER::UAV(Resource ? Resource->GetResource() : nullptr));
    }

    std::string GetResourceStateNameFromState(D3D12_RESOURCE_STATES State)
    {
        static const std::map<D3D12_RESOURCE_STATES, std::string> StateMap = {
            {D3D12_RESOURCE_STATE_COMMON,                     "COMMON"},
            {D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, "VERTEX_AND_CONSTANT_BUFFER"},
            {D3D12_RESOURCE_STATE_INDEX_BUFFER,               "INDEX_BUFFER"},
            {D3D12_RESOURCE_STATE_RENDER_TARGET,              "RENDER_TARGET"},
            {D3D12_RESOURCE_STATE_UNORDERED_ACCESS,           "UNORDERED_ACCESS"},
            {D3D12_RESOURCE_STATE_DEPTH_WRITE,                "DEPTH_WRITE"},
            {D3D12_RESOURCE_STATE_DEPTH_READ,                 "DEPTH_READ"},
            {D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,  "NON_PIXEL_SHADER_RESOURCE"},
            {D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,      "PIXEL_SHADER_RESOURCE"},
            {D3D12_RESOURCE_STATE_STREAM_OUT,                 "STREAM_OUT"},
            {D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,          "INDIRECT_ARGUMENT"},
            {D3D12_RESOURCE_STATE_COPY_DEST,                  "COPY_DEST"},
            {D3D12_RESOURCE_STATE_COPY_SOURCE,                "COPY_SOURCE"},
            {D3D12_RESOURCE_STATE_RESOLVE_DEST,               "RESOLVE_DEST"},
            {D3D12_RESOURCE_STATE_RESOLVE_SOURCE,             "RESOLVE_SOURCE"},
            {D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, "RAYTRACING_ACCELERATION_STRUCTURE"},
            {D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE,        "SHADING_RATE_SOURCE"},
            {D3D12_RESOURCE_STATE_VIDEO_DECODE_READ,          "VIDEO_DECODE_READ"},
            {D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE,         "VIDEO_DECODE_WRITE"},
            {D3D12_RESOURCE_STATE_VIDEO_PROCESS_READ,         "VIDEO_PROCESS_READ"},
            {D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE,        "VIDEO_PROCESS_WRITE"},
            {D3D12_RESOURCE_STATE_VIDEO_ENCODE_READ,          "VIDEO_ENCODE_READ"},
            {D3D12_RESOURCE_STATE_VIDEO_ENCODE_WRITE,         "VIDEO_ENCODE_WRITE"},
            {D3D12_RESOURCE_STATE_PRESENT,                    "PRESENT"},
            {D3D12_RESOURCE_STATE_PREDICATION,                "PREDICATION"},
            {D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,        "ALL_SHADER_RESOURCE"},
            {D3D12_RESOURCE_STATE_GENERIC_READ,               "GENERIC_READ"}
        };


        std::string result = "UNDEFINED";
        auto SearchRes = StateMap.find(State);
        if (SearchRes != StateMap.end())
        {
            result = SearchRes->second;
        }

        return result;
    }
    
}

