#pragma once
#include "d3d12_core.h"
#include "d3d12_fence.h"
#include "d3d12_resource.h"
#include "d3d12_descriptor.h"

namespace RHI
{
    // This class allows for customizable presents
    class IPresent
    {
    public:
        virtual ~IPresent() = default;

        virtual void PrePresent() {}

        virtual void PostPresent() {}
    };

    // HDR is not fully supported yet, still WIP
    // Currently HDR code is based on D3D12HDR Sample from Microsoft Samples
    class D3D12SwapChain : public D3D12DeviceChild
    {
    public:
        enum BitDepth
        {
            _8 = 0,
            _10,
            _16,
            SwapChainBitDepthCount
        };

        enum DisplayCurve
        {
            sRGB = 0, // The display expects an sRGB signal.
            ST2084,   // The display expects an HDR10 signal.
            None      // The display expects a linear signal.
        };

        static constexpr UINT BackBufferCount = 3;

        static constexpr DXGI_FORMAT Format                          = DXGI_FORMAT_R8G8B8A8_UNORM;
        static constexpr DXGI_FORMAT Format_sRGB                     = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        static constexpr DXGI_FORMAT Formats[SwapChainBitDepthCount] = {DXGI_FORMAT_R8G8B8A8_UNORM,
                                                                        DXGI_FORMAT_R10G10B10A2_UNORM,
                                                                        DXGI_FORMAT_R16G16B16A16_FLOAT};

        D3D12SwapChain() noexcept = default;
        explicit D3D12SwapChain(D3D12Device* Parent, HWND HWnd);
        ~D3D12SwapChain();

        D3D12SwapChain(D3D12SwapChain&&) noexcept = default;
        D3D12SwapChain& operator=(D3D12SwapChain&&) noexcept = default;

        D3D12SwapChain(const D3D12SwapChain&) = delete;
        D3D12SwapChain& operator=(const D3D12SwapChain&) = delete;

        void DisplayHDRSupport();

        // DirectX supports two combinations of swapchain pixel formats and colorspaces for HDR content.
        // Option 1: FP16 + DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709
        // Option 2: R10G10B10A2 + DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020
        // Calling this function to ensure the correct color space for the different pixel formats.
        void EnsureSwapChainColorSpace(BitDepth BitDepth, bool EnableST2084);

        [[nodiscard]] D3D12Texture* GetBackBuffer(UINT Index);
        [[nodiscard]] D3D12Texture* GetCurrentBackBufferResource();

        [[nodiscard]] RHIViewport GetViewport() const noexcept;
        [[nodiscard]] RHIRect     GetScissorRect() const noexcept;

        void Resize(UINT Width, UINT Height);

        void Present(bool VSync, IPresent& Present);

    private:
        Microsoft::WRL::ComPtr<IDXGISwapChain4> InitializeSwapChain();

    private:
        HWND WindowHandle   = nullptr;
        UINT Width          = 0;
        UINT Height         = 0;
        bool TearingSupport = false;
        RECT WindowBounds   = {};

        UINT m_CurrentBackBufferIndex = 0;

        Microsoft::WRL::ComPtr<IDXGISwapChain4> p_SwapChain4;
        std::shared_ptr<D3D12Texture>           p_BackBuffers[BackBufferCount];
        
        bool                  HDRSupport        = false;
        bool                  EnableST2084      = false;
        BitDepth              CurrentBitDepth   = _8;
        DisplayCurve          DisplayCurve      = sRGB;
        DXGI_COLOR_SPACE_TYPE CurrentColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;

        D3D12Fence      Fence;
        D3D12SyncHandle SyncHandle;
    };

}