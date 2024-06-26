#pragma once
#include "d3d12_core.h"
#include "d3d12_profiler.h"

#include <shared_mutex>

namespace RHI
{
    class D3D12Device;
    class D3D12Resource;
    class D3D12CommandQueue;
    class GPUDescriptorHeap;
    class CPUDescriptorHeap;
    class DescriptorHeapAllocation;
    class D3D12CommandContext;
    class GraphicsMemory;
    class ResourceUploadBatch;
    
    class D3D12LinkedDevice : public D3D12DeviceChild
    {
    public:
        explicit D3D12LinkedDevice(D3D12Device* Parent, D3D12NodeMask NodeMask);
        ~D3D12LinkedDevice();

        [[nodiscard]] ID3D12Device*        GetDevice() const;
        [[nodiscard]] ID3D12Device5*       GetDevice5() const;
        [[nodiscard]] D3D12NodeMask        GetNodeMask() const noexcept { return NodeMask; }
        [[nodiscard]] D3D12CommandQueue*   GetCommandQueue(RHID3D12CommandQueueType Type);
        [[nodiscard]] D3D12CommandQueue*   GetGraphicsQueue();
        [[nodiscard]] D3D12CommandQueue*   GetAsyncComputeQueue();
        [[nodiscard]] D3D12CommandQueue*   GetCopyQueue1();
        [[nodiscard]] D3D12Profiler*       GetProfiler();
        [[nodiscard]] GPUDescriptorHeap*   GetResourceDescriptorHeap() noexcept;
        [[nodiscard]] GPUDescriptorHeap*   GetSamplerDescriptorHeap() noexcept;
        // clang-format off
		template<typename ViewDesc> CPUDescriptorHeap* GetHeapManager() noexcept;
		template<> CPUDescriptorHeap* GetHeapManager<D3D12_RENDER_TARGET_VIEW_DESC>() noexcept { return m_RtvDescriptorHeaps.get(); }
		template<> CPUDescriptorHeap* GetHeapManager<D3D12_DEPTH_STENCIL_VIEW_DESC>() noexcept { return m_DsvDescriptorHeaps.get(); }
        template<> CPUDescriptorHeap* GetHeapManager<D3D12_CONSTANT_BUFFER_VIEW_DESC>() noexcept { return m_CPUResourceDescriptorHeap.get(); }
		template<> CPUDescriptorHeap* GetHeapManager<D3D12_SHADER_RESOURCE_VIEW_DESC>() noexcept { return m_CPUResourceDescriptorHeap.get(); }
		template<> CPUDescriptorHeap* GetHeapManager<D3D12_UNORDERED_ACCESS_VIEW_DESC>() noexcept { return m_CPUResourceDescriptorHeap.get(); }
        template<> CPUDescriptorHeap* GetHeapManager<D3D12_SAMPLER_DESC>() noexcept { return m_CPUSamplerDescriptorHeap.get(); }

		template<typename ViewDesc> GPUDescriptorHeap* GetDescriptorHeap() noexcept;
        template<> GPUDescriptorHeap* GetDescriptorHeap<D3D12_RENDER_TARGET_VIEW_DESC>() noexcept { return nullptr; }
		template<> GPUDescriptorHeap* GetDescriptorHeap<D3D12_DEPTH_STENCIL_VIEW_DESC>() noexcept { return nullptr; }
		template<> GPUDescriptorHeap* GetDescriptorHeap<D3D12_CONSTANT_BUFFER_VIEW_DESC>() noexcept { return m_ResourceDescriptorHeap.get(); }
		template<> GPUDescriptorHeap* GetDescriptorHeap<D3D12_SHADER_RESOURCE_VIEW_DESC>() noexcept { return m_ResourceDescriptorHeap.get(); }
		template<> GPUDescriptorHeap* GetDescriptorHeap<D3D12_UNORDERED_ACCESS_VIEW_DESC>() noexcept { return m_ResourceDescriptorHeap.get(); }
		template<> GPUDescriptorHeap* GetDescriptorHeap<D3D12_SAMPLER_DESC>() noexcept { return m_SamplerDescriptorHeap.get(); }
        // clang-format on
        [[nodiscard]] D3D12CommandContext* GetCommandContext(UINT ThreadIndex = 0);
        [[nodiscard]] D3D12CommandContext* GetAsyncComputeCommandContext(UINT ThreadIndex = 0);
        [[nodiscard]] D3D12CommandContext* GetCopyContext1();
        [[nodiscard]] D3D12CommandContext* GetCopyContext2();

        [[nodiscard]] GraphicsMemory* GetGraphicsMemory();
        [[nodiscard]] ResourceUploadBatch* GetResourceUploadBatch();

        void OnBeginFrame();
        void OnEndFrame();

        [[nodiscard]] D3D12_RESOURCE_ALLOCATION_INFO GetResourceAllocationInfo(const D3D12_RESOURCE_DESC& Desc) const;
        [[nodiscard]] bool                           ResourceSupport4KBAlignment(D3D12_RESOURCE_DESC& Desc) const;

        void WaitIdle();
        //-------------------------��Դ�ͷ�--------------------------
        void Retire(Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource);
        void Retire(DescriptorHeapAllocation&& Allocation);
        
        void Release(D3D12SyncHandle syncHandle);
        //-----------------------------------------------------------

        static UINT m_FrameIndex;
    private:
        static constexpr UINT MaxSharedBufferCount = 3;

        D3D12NodeMask     NodeMask;
        
        std::shared_ptr<D3D12CommandQueue> m_GraphicsQueue;
        std::shared_ptr<D3D12CommandQueue> m_AsyncComputeQueue;
        std::shared_ptr<D3D12CommandQueue> m_CopyQueue1;
        std::shared_ptr<D3D12CommandQueue> m_CopyQueue2;

        std::shared_ptr<D3D12Profiler> m_Profiler;

        std::shared_ptr<CPUDescriptorHeap> m_RtvDescriptorHeaps;
        std::shared_ptr<CPUDescriptorHeap> m_DsvDescriptorHeaps;
        std::shared_ptr<CPUDescriptorHeap> m_CPUResourceDescriptorHeap;
        std::shared_ptr<CPUDescriptorHeap> m_CPUSamplerDescriptorHeap;
        std::shared_ptr<GPUDescriptorHeap> m_ResourceDescriptorHeap; // D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
        std::shared_ptr<GPUDescriptorHeap> m_SamplerDescriptorHeap;  // D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER


        std::vector<std::shared_ptr<D3D12CommandContext>> m_AvailableCommandContexts;
        std::vector<std::shared_ptr<D3D12CommandContext>> m_AvailableAsyncCommandContexts;
        std::shared_ptr<D3D12CommandContext>              m_CopyContext1;
        std::shared_ptr<D3D12CommandContext>              m_CopyContext2;

        std::shared_ptr<GraphicsMemory>      m_GraphicsMemory;
        std::shared_ptr<ResourceUploadBatch> m_ResourceUploadBatch;

        struct ResourceAllocationInfoTable
        {
            std::shared_mutex                                                        Mutex;
            robin_hood::unordered_map<std::uint64_t, D3D12_RESOURCE_ALLOCATION_INFO> Table;
        } mutable ResourceAllocationInfoTable;

        struct ActiveSharedData
        {
            D3D12SyncHandle                                     m_SyncHandle;
            std::vector<DescriptorHeapAllocation>               m_RetiredAllocations;
            std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_RetiredResource;
        };
        
        ActiveSharedData m_ActiveSharedDatas[MaxSharedBufferCount];
        ActiveSharedData m_CurrentFrameSharedDatas;

        UINT m_CurrentBufferIndex = 0;
    };
}