#include "d3d12_linearAllocator.h"
#include "d3d12_linkedDevice.h"
#include "d3d12_PlatformHelpers.h"

// Set this to 1 to enable some additional debug validation
#define VALIDATE_LISTS 0

#if VALIDATE_LISTS
#include <unordered_set>
#endif

namespace RHI
{
    LinearAllocatorPage::LinearAllocatorPage() noexcept :
        pPrevPage(nullptr), pNextPage(nullptr), mMemory(nullptr), mPendingFence(0), mGpuAddress {}, mOffset(0),
        mSize(0), mRefCount(1)
    {}

    size_t LinearAllocatorPage::Suballocate(_In_ size_t size, _In_ size_t alignment)
    {
        const size_t offset = D3D12RHIUtils::AlignUp(mOffset, alignment);
        if (offset + size > mSize)
        {
            // Use of suballocate should be limited to pages with free space,
            // so really shouldn't happen.
            throw std::runtime_error("LinearAllocatorPage::Suballocate");
        }
        mOffset = offset + size;
        return offset;
    }

    void LinearAllocatorPage::Release() noexcept
    {
        assert(mRefCount > 0);

        if (mRefCount.fetch_sub(1) == 1)
        {
            mUploadResource->Unmap(0, nullptr);
            delete this;
        }
    }

    //--------------------------------------------------------------------------------------
    LinearAllocator::LinearAllocator(_In_ ID3D12Device* pDevice,
                                     _In_ size_t        pageSize,
                                     _In_ size_t        preallocateBytes) noexcept(false) :
        m_pendingPages(nullptr),
        m_usedPages(nullptr), m_unusedPages(nullptr), m_increment(pageSize), m_numPending(0), m_totalPages(0),
        m_fenceCount(0), m_device(pDevice)
    {
        assert(pDevice != nullptr);
#if defined(_DEBUG) || defined(PROFILE)
        m_debugName = L"LinearAllocator";
#endif

        const size_t preallocatePageCount = ((preallocateBytes + pageSize - 1) / pageSize);
        for (size_t preallocatePages = 0; preallocateBytes != 0 && preallocatePages < preallocatePageCount;
             ++preallocatePages)
        {
            if (GetNewPage() == nullptr)
            {
                DebugTrace("LinearAllocator failed to preallocate pages (%zu required bytes, %zu pages)\n",
                           preallocatePageCount * m_increment,
                           preallocatePageCount);
                throw std::bad_alloc();
            }
        }

        ThrowIfFailed(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS((m_fence.ReleaseAndGetAddressOf()))));
    }

    LinearAllocator::~LinearAllocator()
    {
        // Must wait for all pending fences!
        while (m_pendingPages != nullptr)
        {
            RetirePendingPages();
        }

        assert(m_pendingPages == nullptr);

        // Return all the memory
        FreePages(m_unusedPages);
        FreePages(m_usedPages);

        m_pendingPages = nullptr;
        m_usedPages    = nullptr;
        m_unusedPages  = nullptr;
        m_increment    = 0;
    }

    LinearAllocatorPage* LinearAllocator::FindPageForAlloc(_In_ size_t size, _In_ size_t alignment)
    {
#ifdef _DEBUG
        if (size > m_increment)
            throw std::out_of_range("Size must be less or equal to the allocator's increment");
        if (alignment > m_increment)
            throw std::out_of_range("Alignment must be less or equal to the allocator's increment");
        if (size == 0)
            throw std::invalid_argument("Cannot honor zero size allocation request.");
#endif

        auto page = GetPageForAlloc(size, alignment);
        if (!page)
        {
            return nullptr;
        }

        return page;
    }

    // Call this after you submit your work to the driver.
    void LinearAllocator::FenceCommittedPages(_In_ ID3D12CommandQueue* commandQueue)
    {
        // No pending pages
        if (m_usedPages == nullptr)
            return;

        // For all the used pages, fence them
        UINT                 numReady     = 0;
        LinearAllocatorPage* readyPages   = nullptr;
        LinearAllocatorPage* unreadyPages = nullptr;
        LinearAllocatorPage* nextPage     = nullptr;
        for (auto page = m_usedPages; page != nullptr; page = nextPage)
        {
            nextPage = page->pNextPage;

            // Disconnect from the list
            page->pPrevPage = nullptr;

            // This implies the allocator is the only remaining reference to the page, and therefore the memory is ready
            // for re-use.
            if (page->RefCount() == 1)
            {
                // Signal the fence
                numReady++;
                page->mPendingFence = ++m_fenceCount;
                ThrowIfFailed(commandQueue->Signal(m_fence.Get(), m_fenceCount));

                // Link to the ready pages list
                page->pNextPage = readyPages;
                if (readyPages)
                    readyPages->pPrevPage = page;
                readyPages = page;
            }
            else
            {
                // Link to the unready list
                page->pNextPage = unreadyPages;
                if (unreadyPages)
                    unreadyPages->pPrevPage = page;
                unreadyPages = page;
            }
        }

        // Replace the used pages list with the new unready list
        m_usedPages = unreadyPages;

        // Append all those pages from the ready list to the pending list
        if (numReady > 0)
        {
            m_numPending += numReady;
            LinkPageChain(readyPages, m_pendingPages);
        }

#if VALIDATE_LISTS
        ValidatePageLists();
#endif
    }

    // Call this once a frame after all of your driver submissions.
    // (immediately before or after Present-time)
    void LinearAllocator::RetirePendingPages() noexcept
    {
        const uint64_t fenceValue = m_fence->GetCompletedValue();

        // For each page that we know has a fence pending, check it. If the fence has passed,
        // we can mark the page for re-use.
        auto page = m_pendingPages;
        while (page != nullptr)
        {
            auto nextPage = page->pNextPage;

            assert(page->mPendingFence != 0);

            if (fenceValue >= page->mPendingFence)
            {
                // Fence has passed. It is safe to use this page again.
                ReleasePage(page);
            }

            page = nextPage;
        }
    }

    void LinearAllocator::Shrink() noexcept
    {
        FreePages(m_unusedPages);
        m_unusedPages = nullptr;

#if VALIDATE_LISTS
        ValidatePageLists();
#endif
    }

    LinearAllocatorPage* LinearAllocator::GetCleanPageForAlloc()
    {
        // Grab the first unused page, if one exists. Else, allocate a new page.
        auto page = m_unusedPages;
        if (!page)
        {
            // Allocate a new page
            page = GetNewPage();
            if (!page)
            {
                return nullptr;
            }
        }

        // Mark this page as used
        UnlinkPage(page);
        LinkPage(page, m_usedPages);

        assert(page->mOffset == 0);

        return page;
    }

    LinearAllocatorPage* LinearAllocator::GetPageForAlloc(size_t sizeBytes, size_t alignment)
    {
        // Fast path
        if (sizeBytes == m_increment && (alignment == 0 || alignment == m_increment))
        {
            return GetCleanPageForAlloc();
        }

        // Find a page in the pending pages list that has space.
        auto page = FindPageForAlloc(m_usedPages, sizeBytes, alignment);
        if (!page)
        {
            page = GetCleanPageForAlloc();
        }

        return page;
    }

    LinearAllocatorPage* LinearAllocator::FindPageForAlloc(LinearAllocatorPage* list, size_t sizeBytes, size_t alignment) noexcept
    {
        for (auto page = list; page != nullptr; page = page->pNextPage)
        {
            const size_t offset = D3D12RHIUtils::AlignUp(page->mOffset, alignment);
            if (offset + sizeBytes <= m_increment)
                return page;
        }
        return nullptr;
    }

    LinearAllocatorPage* LinearAllocator::GetNewPage()
    {
        const CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
        const CD3DX12_RESOURCE_DESC   bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_increment);

        // Allocate the upload heap
        Microsoft::WRL::ComPtr<ID3D12Resource> spResource;
        HRESULT hr = m_device->CreateCommittedResource(&uploadHeapProperties,
                                                       D3D12_HEAP_FLAG_NONE,
                                                       &bufferDesc,
                                                       D3D12_RESOURCE_STATE_GENERIC_READ,
                                                       nullptr,
                                                       IID_PPV_ARGS(spResource.ReleaseAndGetAddressOf()));
        if (FAILED(hr))
        {
            if (hr != E_OUTOFMEMORY)
            {
                DebugTrace("LinearAllocator::GetNewPage resource allocation failed due to unexpected error %08X\n",
                           static_cast<unsigned int>(hr));
            }
            return nullptr;
        }

#if defined(_DEBUG) || defined(PROFILE)
        spResource->SetName(m_debugName.empty() ? L"LinearAllocator" : m_debugName.c_str());
#endif

        // Get a pointer to the memory
        void* pMemory = nullptr;
        ThrowIfFailed(spResource->Map(0, nullptr, &pMemory));
        memset(pMemory, 0, m_increment);

        // Add the page to the page list
        auto page         = new LinearAllocatorPage;
        page->mMemory     = pMemory;
        page->mGpuAddress = spResource->GetGPUVirtualAddress();
        page->mSize       = m_increment;
        page->mUploadResource.Swap(spResource);

        // Set as head of the list
        page->pNextPage = m_unusedPages;
        if (m_unusedPages)
            m_unusedPages->pPrevPage = page;
        m_unusedPages = page;
        m_totalPages++;

#if VALIDATE_LISTS
        ValidatePageLists();
#endif

        return page;
    }

    void LinearAllocator::UnlinkPage(LinearAllocatorPage* page) noexcept
    {
        if (page->pPrevPage)
            page->pPrevPage->pNextPage = page->pNextPage;

        // Check that it isn't the head of any of our tracked lists
        else if (page == m_unusedPages)
            m_unusedPages = page->pNextPage;
        else if (page == m_usedPages)
            m_usedPages = page->pNextPage;
        else if (page == m_pendingPages)
            m_pendingPages = page->pNextPage;

        if (page->pNextPage)
            page->pNextPage->pPrevPage = page->pPrevPage;

        page->pNextPage = nullptr;
        page->pPrevPage = nullptr;

#if VALIDATE_LISTS
        ValidatePageLists();
#endif
    }

    void LinearAllocator::LinkPageChain(LinearAllocatorPage* page, LinearAllocatorPage*& list) noexcept
    {
#if VALIDATE_LISTS
        // Walk the chain and ensure it's not in the list twice
        for (LinearAllocatorPage* cur = list; cur != nullptr; cur = cur->pNextPage)
        {
            assert(cur != page);
        }
#endif
        assert(page->pPrevPage == nullptr);
        assert(list == nullptr || list->pPrevPage == nullptr);

        // Follow chain to the end and append
        LinearAllocatorPage* lastPage = nullptr;
        for (lastPage = page; lastPage->pNextPage != nullptr; lastPage = lastPage->pNextPage)
        {}

        lastPage->pNextPage = list;
        if (list)
            list->pPrevPage = lastPage;

        list = page;

#if VALIDATE_LISTS
        ValidatePageLists();
#endif
    }

    void LinearAllocator::LinkPage(LinearAllocatorPage* page, LinearAllocatorPage*& list) noexcept
    {
#if VALIDATE_LISTS
        // Walk the chain and ensure it's not in the list twice
        for (LinearAllocatorPage* cur = list; cur != nullptr; cur = cur->pNextPage)
        {
            assert(cur != page);
        }
#endif
        assert(page->pNextPage == nullptr);
        assert(page->pPrevPage == nullptr);
        assert(list == nullptr || list->pPrevPage == nullptr);

        page->pNextPage = list;
        if (list)
            list->pPrevPage = page;

        list = page;

#if VALIDATE_LISTS
        ValidatePageLists();
#endif
    }

    void LinearAllocator::ReleasePage(LinearAllocatorPage* page) noexcept
    {
        assert(m_numPending > 0);
        m_numPending--;

        UnlinkPage(page);
        LinkPage(page, m_unusedPages);

        // Reset the page offset (effectively erasing the memory)
        page->mOffset = 0;

#ifdef _DEBUG
        memset(page->mMemory, 0, m_increment);
#endif

#if VALIDATE_LISTS
        ValidatePageLists();
#endif
    }

    void LinearAllocator::FreePages(LinearAllocatorPage* page) noexcept
    {
        while (page != nullptr)
        {
            auto nextPage = page->pNextPage;

            page->Release();

            page = nextPage;
            assert(m_totalPages > 0);
            m_totalPages--;
        }
    }

    #if VALIDATE_LISTS
    void LinearAllocator::ValidateList(LinearAllocatorPage* list)
    {
        for (auto page = list, *lastPage = nullptr; page != nullptr; lastPage = page, page = page->pNextPage)
        {
            if (page->pPrevPage != lastPage)
            {
                throw std::runtime_error("Broken link to previous");
            }
        }
    }

    void LinearAllocator::ValidatePageLists()
    {
        ValidateList(m_pendingPages);
        ValidateList(m_usedPages);
        ValidateList(m_unusedPages);
    }
#endif

    #if defined(_DEBUG) || defined(PROFILE)
    void LinearAllocator::SetDebugName(const char* name)
    {
        wchar_t   wname[MAX_PATH] = {};
        const int result = MultiByteToWideChar(65001, 0, name, static_cast<int>(strlen(name)), wname, MAX_PATH);
        if (result > 0)
        {
            SetDebugName(wname);
        }
    }

    void LinearAllocator::SetDebugName(const wchar_t* name)
    {
        m_debugName = name;

        // Rename existing pages
        m_fence->SetName(name);
        SetPageDebugName(m_pendingPages);
        SetPageDebugName(m_usedPages);
        SetPageDebugName(m_unusedPages);
    }

    void LinearAllocator::SetPageDebugName(LinearAllocatorPage* list) noexcept
    {
        for (auto page = list; page != nullptr; page = page->pNextPage)
        {
            page->mUploadResource->SetName(m_debugName.c_str());
        }
    }
#endif













































































    
//    D3D12LinearAllocatorPage::D3D12LinearAllocatorPage(Microsoft::WRL::ComPtr<ID3D12Resource> Resource,
//                                                       UINT64                                 PageSize) :
//        Resource(Resource),
//        Offset(0), PageSize(PageSize)
//    {
//        Resource->Map(0, nullptr, reinterpret_cast<void**>(&CpuVirtualAddress));
//        GpuVirtualAddress = Resource->GetGPUVirtualAddress();
//    }
//
//	D3D12LinearAllocatorPage::~D3D12LinearAllocatorPage() { Resource->Unmap(0, nullptr); }
//
//    std::optional<D3D12Allocation> D3D12LinearAllocatorPage::Suballocate(UINT64 Size, UINT Alignment)
//    {
//        UINT64 AlignedSize = D3D12RHIUtils::AlignUp(Size, static_cast<UINT64>(Alignment));
//        if (Offset + AlignedSize > this->PageSize)
//        {
//            return std::nullopt;
//        }
//
//        D3D12Allocation Allocation = {Resource.Get(),
//                                      Offset,
//                                      Size,
//                                      CpuVirtualAddress + Offset,
//                                      GpuVirtualAddress + Offset};
//        Offset += AlignedSize;
//        return Allocation;
//    }
//
//    void D3D12LinearAllocatorPage::Reset() { Offset = 0; }
//
//	D3D12LinearAllocator::D3D12LinearAllocator(D3D12LinkedDevice* Parent) : D3D12LinkedDeviceChild(Parent) {}
//
//    void D3D12LinearAllocator::Version(D3D12SyncHandle SyncHandle)
//    {
//        if (!CurrentPage)
//        {
//            return;
//        }
//
//        this->SyncHandle = SyncHandle;
//
//        RetiredPageList.push_back(std::exchange(CurrentPage, nullptr));
//        DiscardPages(SyncHandle.GetValue(), RetiredPageList);
//        RetiredPageList.clear();
//    }
//
//    D3D12Allocation D3D12LinearAllocator::Allocate(UINT64 Size,
//                                                   UINT Alignment /*= D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT*/)
//    {
//        if (!CurrentPage)
//        {
//            CurrentPage = RequestPage();
//        }
//
//        std::optional<D3D12Allocation> OptAllocation = CurrentPage->Suballocate(Size, Alignment);
//
//        if (!OptAllocation)
//        {
//            RetiredPageList.push_back(CurrentPage);
//
//            CurrentPage   = RequestPage();
//            OptAllocation = CurrentPage->Suballocate(Size, Alignment);
//            assert(OptAllocation.has_value());
//        }
//
//        return OptAllocation.value();
//    }
//
//    D3D12LinearAllocatorPage* D3D12LinearAllocator::RequestPage()
//    {
//        while (SyncHandle && !RetiredPages.empty() && RetiredPages.front().first <= SyncHandle.GetValue())
//        {
//            AvailablePages.push(RetiredPages.front().second);
//            RetiredPages.pop();
//        }
//
//        D3D12LinearAllocatorPage* Page = nullptr;
//
//        if (!AvailablePages.empty())
//        {
//            Page = AvailablePages.front();
//            Page->Reset();
//            AvailablePages.pop();
//        }
//        else
//        {
//            Page = PagePool.emplace_back(CreateNewPage(CpuAllocatorPageSize)).get();
//        }
//
//        return Page;
//    }
//
//    std::unique_ptr<D3D12LinearAllocatorPage> D3D12LinearAllocator::CreateNewPage(UINT64 PageSize) const
//    {
//        auto HeapProperties =
//            CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD, Parent->GetNodeMask(), Parent->GetNodeMask());
//        auto ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(PageSize);
//
//        Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
//        VERIFY_D3D12_API(
//            GetParentLinkedDevice()->GetDevice()->CreateCommittedResource(&HeapProperties,
//                                                                          D3D12_HEAP_FLAG_NONE,
//                                                                          &ResourceDesc,
//                                                                          D3D12_RESOURCE_STATE_GENERIC_READ,
//                                                                          nullptr,
//                                                                          IID_PPV_ARGS(&Resource)));
//
//#ifdef _DEBUG
//        Resource->SetName(L"Linear Allocator Page");
//#endif
//
//        return std::make_unique<D3D12LinearAllocatorPage>(Resource, PageSize);
//    }
//
//    void D3D12LinearAllocator::DiscardPages(UINT64 FenceValue, const std::vector<D3D12LinearAllocatorPage*>& Pages)
//    {
//        for (const auto& Page : Pages)
//        {
//            RetiredPages.push(std::make_pair(FenceValue, Page));
//        }
//    }
}
