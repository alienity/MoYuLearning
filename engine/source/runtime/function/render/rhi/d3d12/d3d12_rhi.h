#pragma once

// win32
#define WIN32_LEAN_AND_MEAN

#define NOGDICAPMASKS     // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define NOVIRTUALKEYCODES // VK_*
//#define NOWINMESSAGES	  // WM_*, EM_*, LB_*, CB_*
#define NOWINSTYLES   // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
#define NOSYSMETRICS  // SM_*
#define NOMENUS       // MF_*
#define NOICONS       // IDI_*
#define NOKEYSTATES   // MK_*
#define NOSYSCOMMANDS // SC_*
#define NORASTEROPS   // Binary and Tertiary raster ops
#define NOSHOWWINDOW  // SW_*
#define OEMRESOURCE   // OEM Resource values
#define NOATOM        // Atom Manager routines
#define NOCLIPBOARD   // Clipboard routines
#define NOCOLOR       // Screen colors
//#define NOCTLMGR		  // Control and Dialog routines
#define NODRAWTEXT // DrawText() and DT_*
#define NOGDI      // All GDI defines and routines
//#define NOKERNEL		  // All KERNEL defines and routines
//#define NOUSER			  // All USER defines and routines
#define NONLS      // All NLS defines and routines
#define NOMB       // MB_* and MessageBox()
#define NOMEMMGR   // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE // typedef METAFILEPICT
//#define NOMINMAX   // Macros min(a,b) and max(a,b)
//#define NOMSG			  // typedef MSG and associated routines
#define NOOPENFILE       // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL         // SB_* and scrolling routines
#define NOSERVICE        // All Service Controller routines, SERVICE_ equates, etc.
#define NOSOUND          // Sound driver routines
#define NOTEXTMETRIC     // typedef TEXTMETRIC and associated routines
#define NOWH             // SetWindowsHook and WH_*
#define NOWINOFFSETS     // GWL_*, GCL_*, associated routines
#define NOCOMM           // COMM driver routines
#define NOKANJI          // Kanji support stuff.
#define NOHELP           // Help engine interface.
#define NOPROFILER       // Profiler interface.
#define NODEFERWINDOWPOS // DeferWindowPos routines
#define NOMCX            // Modem Configuration Extensions

#ifndef NOMINMAX
#define NOMINMAX
#endif // !NOMINMAX
#include <Windows.h>

// DXGI
#include <dxgi1_6.h>
#if defined(_DEBUG)
#include <dxgidebug.h>
#endif
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

#define D3D12_GPU_VIRTUAL_ADDRESS_NULL    ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

// D3D12
#include "directx/d3d12.h"
#include "directx/d3dx12.h"
#include "directx/d3d12sdklayers.h"
#include "directx/d3d12shader.h"
#pragma comment(lib, "d3d12.lib")
#include <pix3.h>

#include "dstorage.h"

#include <string_view>

#include <optional>
#include <filesystem>

#include <bitset>
#include <algorithm>
#include <atomic>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <exception>
#include <initializer_list>
#include <iterator>
#include <list>
#include <map>
#include <queue>
#include <memory>
#include <mutex>
#include <new>
#include <set>
#include <stdexcept>
#include <string>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#pragma warning(push)
#pragma warning(disable : 5204 5220)
#include <future>
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable : 4702)
#include <functional>
#pragma warning(pop)

#include <malloc.h>

#pragma warning(push)
#pragma warning(disable : 4467 5038 5204 5220)
#ifdef __MINGW32__
#include <wrl/client.h>
#else
#include <wrl.h>
#endif
#pragma warning(pop)

#include <wincodec.h>

#define VERIFY_D3D12_API(expr) \
    do \
    { \
        HRESULT hr = expr; \
        if (FAILED(hr)) \
        { \
            throw RHI::D3D12Exception(__LINE__, hr); \
        } \
    } while (false)

#define D3D12Concatenate(a, b) a##b
#define D3D12GetScopedEventVariableName(a, b) D3D12Concatenate(a, b)
#define D3D12ScopedEvent(context, name) \
    RHI::D3D12ScopedEventObject D3D12GetScopedEventVariableName(D3D12Event, __LINE__)(context, name)

// Custom resource states
constexpr D3D12_RESOURCE_STATES D3D12_RESOURCE_STATE_UNKNOWN       = static_cast<D3D12_RESOURCE_STATES>(-1);
constexpr D3D12_RESOURCE_STATES D3D12_RESOURCE_STATE_UNINITIALIZED = static_cast<D3D12_RESOURCE_STATES>(-2);

// D3D12 Config
#define MOYU_RHI_D3D12_DEBUG_RESOURCE_STATES
#define MOYU_RHI_D3D12_ENHANCED_BARRIER

#define MOYU_RHI_D3D12_BUILTIN_TRIANGLE_INTERSECTION_ATTRIBUTES (8)

// Root signature entry cost: https://docs.microsoft.com/en-us/windows/win32/direct3d12/root-signature-limits
// Local root descriptor table cost:
// https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#shader-table-memory-initialization
#define MOYU_RHI_D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_COST (1)
#define MOYU_RHI_D3D12_LOCAL_ROOT_DESCRIPTOR_TABLE_COST (2)
#define MOYU_RHI_D3D12_ROOT_CONSTANT_COST (1)
#define MOYU_RHI_D3D12_ROOT_DESCRIPTOR_COST (2)

#define MOYU_RHI_D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT \
    ((D3D12_MAX_ROOT_COST) / (MOYU_RHI_D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_COST))
