#pragma once
#include "d3d12_core.h"
#include "d3d12_linkedDevice.h"
#include "d3d12_resource.h"
#include "d3d12_commandList.h"
#include "d3d12_graphicsMemory.h"
#include "d3d12_rootSignature.h"
#include "d3d12_commandSignature.h"
#include "d3d12_dynamicDescriptorHeap.h"
#include "d3d12_pipelineState.h"
#include "d3d12_profiler.h"

namespace RHI
{
    class D3D12CommandQueue;
    
    class D3D12RenderTargetView;
    class D3D12DepthStencilView;
    class D3D12ShaderResourceView;
    class D3D12UnorderedAccessView;
    class D3D12GraphicsContext;
    class D3D12ComputeContext;

    class D3D12CommandAllocatorPool : public D3D12LinkedDeviceChild
    {
    public:
        D3D12CommandAllocatorPool() noexcept = default;
        explicit D3D12CommandAllocatorPool(D3D12LinkedDevice* Parent, D3D12_COMMAND_LIST_TYPE CommandListType) noexcept;

        [[nodiscard]] Microsoft::WRL::ComPtr<ID3D12CommandAllocator> RequestCommandAllocator();

        void DiscardCommandAllocator(Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& CommandAllocator, D3D12SyncHandle SyncHandle);

    private:
        D3D12_COMMAND_LIST_TYPE                                    m_CommandListType;
        CFencePool<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> m_CommandAllocatorPool;
    };

    // clang-format off
	class D3D12CommandContext : public D3D12LinkedDeviceChild
    {
    public:
        D3D12CommandContext() noexcept = default;
        explicit D3D12CommandContext(D3D12LinkedDevice* Parent, RHID3D12CommandQueueType Type, D3D12_COMMAND_LIST_TYPE CommandListType);

        ~D3D12CommandContext();

        D3D12CommandContext(D3D12CommandContext&&) noexcept = default;
        D3D12CommandContext& operator=(D3D12CommandContext&&) noexcept = default;

        D3D12CommandContext(const D3D12CommandContext&) = delete;
        D3D12CommandContext& operator=(const D3D12CommandContext&) = delete;

        [[nodiscard]] D3D12CommandQueue*          GetCommandQueue() const noexcept;
        [[nodiscard]] ID3D12GraphicsCommandList*  GetGraphicsCommandList() const noexcept;
        [[nodiscard]] ID3D12GraphicsCommandList4* GetGraphicsCommandList4() const noexcept;
        [[nodiscard]] ID3D12GraphicsCommandList6* GetGraphicsCommandList6() const noexcept;
        D3D12CommandListHandle&                   operator->() { return m_CommandListHandle; }

        D3D12GraphicsContext* GetGraphicsContext();
        D3D12ComputeContext* GetComputeContext();

        static std::shared_ptr<D3D12CommandContext> Begin(D3D12LinkedDevice* Parent, RHID3D12CommandQueueType Type);
        // Close and Execute combine to one function
        D3D12SyncHandle Finish(bool WaitForCompletion = true);

        void Open();
        void Close();

        // Returns D3D12SyncHandle, may be ignored if WaitForCompletion is true
        D3D12SyncHandle Execute(bool WaitForCompletion);

        void CopyBuffer(D3D12Buffer* Dest, D3D12Buffer* Src);
        void CopyBufferRegion(D3D12Buffer* Dest, UINT64 DestOffset, D3D12Buffer* Src, UINT64 SrcOffset, UINT64 NumBytes);
        void CopySubresource(D3D12Texture* Dest, uint32_t DestSubIndex, D3D12Texture* Src, uint32_t SrcSubIndex);
        void CopyTextureRegion(D3D12Texture* Dest, uint32_t x, uint32_t y, uint32_t z, D3D12Texture* Source, RECT& rect);
        void ResetCounter(D3D12Buffer* CounterResource, UINT64 CounterOffset = 0, uint32_t Value = 0);

        // Creates a readback buffer of sufficient size, copies the texture into it,
        // and returns row pitch in bytes.
        //uint32_t ReadbackTexture(ReadbackBuffer* DstBuffer, D3D12Texture* SrcBuffer);

        GraphicsResource ReserveUploadMemory(UINT64 SizeInBytes, uint32_t Alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        
        std::shared_ptr<D3D12Buffer> ReverveTmpBuffer(UINT64 SizeInBytes);
        std::shared_ptr<D3D12Texture> ReverveTmpTexture(RHIRenderSurfaceBaseDesc SurfaceDesc);

        static void InitializeTexture(D3D12LinkedDevice* Parent, D3D12Texture* Dest, std::vector<D3D12_SUBRESOURCE_DATA> Subresources);
        static void InitializeTexture(D3D12LinkedDevice* Parent, D3D12Texture* Dest, uint32_t FirstSubresource, std::vector<D3D12_SUBRESOURCE_DATA> Subresources);
        static void InitializeBuffer(D3D12LinkedDevice* Parent, D3D12Buffer* Dest, const void* Data, UINT64 NumBytes, UINT64 DestOffset = 0);
        static void InitializeTextureArraySlice(D3D12LinkedDevice* Parent, D3D12Texture* Dest, uint32_t SliceIndex, D3D12Texture* Src);

        void WriteBuffer(D3D12Buffer* Dest, UINT64 DestOffset, const void* BufferData, UINT64 NumBytes);
        void FillBuffer(D3D12Buffer* Dest, UINT64 DestOffset, DWParam Value, UINT64 NumBytes);

        void TransitionBarrier(D3D12Resource* Resource, D3D12_RESOURCE_STATES State, uint32_t Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool FlushImmediate = false);
        void AliasingBarrier(D3D12Resource* BeforeResource, D3D12Resource* AfterResource, bool FlushImmediate = false);
        void InsertUAVBarrier(D3D12Resource* Resource, bool FlushImmediate = false);
        void FlushResourceBarriers();

        bool AssertResourceState(D3D12Resource* Resource, D3D12_RESOURCE_STATES State, uint32_t Subresource);

        /*
        void InsertTimeStamp( ID3D12QueryHeap* pQueryHeap, uint32_t QueryIdx );
        void ResolveTimeStamps( ID3D12Resource* pReadbackHeap, ID3D12QueryHeap* pQueryHeap, uint32_t NumQueries );
        void PIXBeginEvent(const wchar_t* label);
        void PIXEndEvent(void);
        void PIXSetMarker(const wchar_t* label);
        */

        void SetPipelineState(D3D12PipelineState* PipelineState);
        void SetPipelineState(D3D12RaytracingPipelineState* RaytracingPipelineState);

        void SetPredication(D3D12Buffer* Buffer, UINT64 BufferOffset, D3D12_PREDICATION_OP Op);

        void DispatchRays(const D3D12_DISPATCH_RAYS_DESC* pDesc);

        void DispatchMesh(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ);

    //protected:
    //    template<RHI_PIPELINE_STATE_TYPE PsoType>
    //    void SetDynamicResourceDescriptorTables(D3D12RootSignature* RootSignature);

    protected:
        RHID3D12CommandQueueType                       m_Type;
        D3D12_COMMAND_LIST_TYPE                        m_CommandListType;
        D3D12CommandListHandle                         m_CommandListHandle;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
        D3D12CommandAllocatorPool                      m_CommandAllocatorPool;
        GraphicsMemory*                                m_GraphicsMemory;
        
        // Every context must use its own allocator that maintains individual list of retired descriptor heaps to
        // avoid interference with other command contexts
        // The allocations in heaps are discarded at the end of the frame.
        std::shared_ptr<RHI::DynamicDescriptorHeap> m_pDynamicViewDescriptorHeap;	// HEAP_TYPE_CBV_SRV_UAV
        std::shared_ptr<RHI::DynamicDescriptorHeap> m_pDynamicSamplerDescriptorHeap;	// HEAP_TYPE_SAMPLER

        // Cached Resource
        std::vector<std::shared_ptr<D3D12Buffer>> m_CachedBufferes;
        std::vector<std::shared_ptr<D3D12Texture>> m_CachedTextures;

        // TODO: Finish cache
        // State Cache
        UINT64 DirtyStates = 0xffffffff;
        struct StateCache
        {
            D3D12RootSignature* RootSignature;
            D3D12PipelineState* PipelineState;

            struct
            {
                // Viewport
                uint32_t           NumViewports                                                        = 0;
                D3D12_VIEWPORT Viewports[D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};

                // Scissor Rect
                uint32_t       NumScissorRects                                                        = 0;
                D3D12_RECT ScissorRects[D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
            } Graphics;

            struct
            {
            } Compute;

            struct
            {
                D3D12RaytracingPipelineState* RaytracingPipelineState;
            } Raytracing;
        } Cache = {};
    };

    class D3D12GraphicsContext : public D3D12CommandContext
    {
    public:
        void ClearUAV(RHI::D3D12Buffer* Target);
        void ClearUAV(RHI::D3D12Texture* Target);
        void ClearColor(RHI::D3D12Texture* Target, D3D12_RECT* Rect = nullptr);
        void ClearColor(RHI::D3D12Texture* Target, float Colour[4], D3D12_RECT* Rect = nullptr);
        void ClearDepth(RHI::D3D12Texture* Target);
        void ClearStencil(RHI::D3D12Texture* Target);
        void ClearDepthAndStencil(RHI::D3D12Texture* Target);

        void ClearRenderTarget(D3D12RenderTargetView* RenderTargetView, D3D12DepthStencilView* DepthStencilView);
        void ClearRenderTarget(std::vector<D3D12RenderTargetView*> RenderTargetViews, D3D12DepthStencilView* DepthStencilView);
        
        void BeginQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, uint32_t HeapIndex);
        void EndQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, uint32_t HeapIndex);
        void ResolveQueryData(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, uint32_t StartIndex, uint32_t NumQueries, ID3D12Resource* DestinationBuffer, UINT64 DestinationBufferOffset);

        void SetRootSignature(D3D12RootSignature* RootSignature);

        void SetRenderTargets(std::vector<D3D12RenderTargetView*> RenderTargetViews);
        void SetRenderTargets(std::vector<D3D12RenderTargetView*> RenderTargetViews, D3D12DepthStencilView* DepthStencilView);
        void SetRenderTarget(D3D12RenderTargetView* RenderTargetView);
        void SetRenderTarget(D3D12RenderTargetView* RenderTargetView, D3D12DepthStencilView* DepthStencilView);
        void SetDepthStencilTarget(D3D12DepthStencilView* DepthStencilView) { SetRenderTarget(nullptr, DepthStencilView); }

        void SetViewport(const RHIViewport& Viewport);
        void SetViewport(FLOAT TopLeftX, FLOAT TopLeftY, FLOAT Width, FLOAT Height, FLOAT MinDepth = 0.0f, FLOAT MaxDepth = 1.0f);
        void SetViewports(std::vector<RHIViewport> Viewports);
        void SetScissorRect(const RHIRect& ScissorRect);
        void SetScissorRect(uint32_t Left, uint32_t Top, uint32_t Right, uint32_t Bottom);
        void SetScissorRects(std::vector<RHIRect> ScissorRects);
        void SetViewportAndScissorRect(const RHIViewport& vp, const RHIRect& rect);
        void SetViewportAndScissorRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
        void SetStencilRef(uint32_t StencilRef);
        void SetBlendFactor(MoYu::Color BlendFactor);
        void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY Topology);

        void SetConstantArray(uint32_t RootIndex, uint32_t Offset, uint32_t NumConstants, const void* pConstants);
        void SetConstantArray(uint32_t RootIndex, uint32_t NumConstants, const void* pConstants);
        void SetConstant(uint32_t RootIndex, uint32_t Offset, DWParam Val);
        void SetConstants(uint32_t RootIndex, DWParam X);
        void SetConstants(uint32_t RootIndex, DWParam X, DWParam Y);
        void SetConstants(uint32_t RootIndex, DWParam X, DWParam Y, DWParam Z);
        void SetConstants(uint32_t RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W);
        void SetConstantBuffer(uint32_t RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV);
        void SetDynamicConstantBufferView(uint32_t RootParameterIndex, UINT64 BufferSize, const void* BufferData);
        template<typename T>
        void SetDynamicConstantBufferView(uint32_t RootParameterIndex, const T& Data) { SetConstantBuffer(RootParameterIndex, sizeof(T), &Data); }
        void SetBufferSRV(uint32_t RootIndex, D3D12Buffer* BufferSRV, UINT64 Offset = 0);
        void SetBufferUAV(uint32_t RootIndex, D3D12Buffer* BufferUAV, UINT64 Offset = 0);
        void SetDescriptorTable(uint32_t RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle);

        void SetDynamicDescriptor(uint32_t RootIndex, uint32_t Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle);
        void SetDynamicDescriptors(uint32_t RootIndex, uint32_t Offset, uint32_t Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);
        void SetDynamicSampler(uint32_t RootIndex, uint32_t Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle);
        void SetDynamicSamplers(uint32_t RootIndex, uint32_t Offset, uint32_t Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);

        void SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& IBView);
        void SetVertexBuffer(uint32_t Slot, const D3D12_VERTEX_BUFFER_VIEW& VBView);
        void SetVertexBuffers(uint32_t StartSlot, uint32_t Count, const D3D12_VERTEX_BUFFER_VIEW VBViews[]);
        void SetDynamicVB(uint32_t Slot, UINT64 NumVertices, UINT64 VertexStride, const void* VBData);
        void SetDynamicIB(UINT64 IndexCount, const UINT16* IBData);
        void SetDynamicSRV(uint32_t RootIndex, UINT64 BufferSize, const void* BufferData);

        void Draw(uint32_t VertexCount, uint32_t VertexStartOffset = 0);
        void DrawIndexed(uint32_t IndexCount, uint32_t StartIndexLocation = 0, INT BaseVertexLocation = 0);
        void DrawInstanced(uint32_t VertexCount, uint32_t InstanceCount, uint32_t StartVertexLocation, uint32_t StartInstanceLocation);
        void DrawIndexedInstanced(uint32_t IndexCount, uint32_t InstanceCount, uint32_t StartIndexLocation, INT  BaseVertexLocation, uint32_t StartInstanceLocation);
        void DrawIndirect(D3D12Buffer* ArgumentBuffer, UINT64 ArgumentBufferOffset = 0);
        void ExecuteIndirect(D3D12CommandSignature* CommandSig, D3D12Buffer* ArgumentBuffer, UINT64 ArgumentStartOffset = 0,
            UINT32 MaxCommands = 1, D3D12Buffer* CommandCounterBuffer = nullptr, UINT64 CounterOffset = 0);

    private:
    };

    class D3D12ComputeContext : public D3D12CommandContext
    {
    public:
        void ClearUAV(RHI::D3D12Buffer* Target);
        void ClearUAV(RHI::D3D12Texture* Target);

        void SetRootSignature(D3D12RootSignature* RootSignature);

        void SetConstantArray(uint32_t RootIndex, uint32_t NumConstants, const void* pConstants);
        void SetConstant(uint32_t RootIndex, uint32_t Offset, DWParam Val);
        void SetConstants(uint32_t RootIndex, DWParam X);
        void SetConstants(uint32_t RootIndex, DWParam X, DWParam Y);
        void SetConstants(uint32_t RootIndex, DWParam X, DWParam Y, DWParam Z);
        void SetConstants(uint32_t RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W);
        void SetConstantBuffer(uint32_t RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV);
        void SetDynamicConstantBufferView(uint32_t RootParameterIndex, UINT64 BufferSize, const void* BufferData);
        template<typename T>
        void SetDynamicConstantBufferView(uint32_t RootParameterIndex, const T& Data)
        {
            SetDynamicConstantBufferView(RootParameterIndex, sizeof(T), &Data);
        }
        void SetDynamicSRV(uint32_t RootIndex, UINT64 BufferSize, const void* BufferData);
        void SetBufferSRV(uint32_t RootIndex, D3D12Buffer* BufferSRV, UINT64 Offset = 0);
        void SetBufferUAV(uint32_t RootIndex, D3D12Buffer* BufferUAV, UINT64 Offset = 0);
        void SetDescriptorTable(uint32_t RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle);

        void SetDynamicDescriptor(uint32_t RootIndex, uint32_t Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle);
        void SetDynamicDescriptors(uint32_t RootIndex, uint32_t Offset, uint32_t Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);
        void SetDynamicSampler(uint32_t RootIndex, uint32_t Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle);
        void SetDynamicSamplers(uint32_t RootIndex, uint32_t Offset, uint32_t Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);
        
        void Dispatch(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ);
        void Dispatch1D(UINT64 ThreadCountX, UINT64 GroupSizeX = 64);
        void Dispatch2D(UINT64 ThreadCountX, UINT64 ThreadCountY, UINT64 GroupSizeX = 8, UINT64 GroupSizeY = 8);
        void Dispatch3D(UINT64 ThreadCountX, UINT64 ThreadCountY, UINT64 ThreadCountZ, UINT64 GroupSizeX, UINT64 GroupSizeY, UINT64 GroupSizeZ);
        void DispatchIndirect(D3D12Buffer* ArgumentBuffer, UINT64 ArgumentBufferOffset = 0);
        void ExecuteIndirect(D3D12CommandSignature* CommandSig, D3D12Buffer* ArgumentBuffer, UINT64 ArgumentStartOffset = 0,
            UINT32 MaxCommands = 1, D3D12Buffer* CommandCounterBuffer = nullptr, UINT64 CounterOffset = 0);

    private:
        template<typename T>
        constexpr T RoundUpAndDivide(T Value, size_t Alignment)
        {
            return (T)((Value + Alignment - 1) / Alignment);
        }
    };

    // clang-format on

    class D3D12ScopedEventObject
    {
    public:
        D3D12ScopedEventObject(D3D12CommandContext* CommandContext, std::string_view Name);

    private:
        D3D12ProfileBlock ProfileBlock;
#ifdef _DEBUG
        PIXScopedEventObject<ID3D12GraphicsCommandList> PixEvent;
#endif
    };
}