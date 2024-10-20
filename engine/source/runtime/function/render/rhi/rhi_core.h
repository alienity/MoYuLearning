#pragma once
#include <cstddef>
#include <cassert>

#include <string>
#include <string_view>

#include <memory>
#include <mutex>
#include <optional>
#include <filesystem>

#include <array>
#include <vector>
#include <map>
#include <unordered_map>
#include <queue>
#include <set>

#include "directx/dxgiformat.h"
#include "directx/d3d12.h"

#include "runtime/core/base/macro.h"
#include "runtime/function/global/global_context.h"
#include "runtime/core/math/moyu_math2.h"
#include "runtime/platform/system/system_core.h"

enum class RHI_VENDOR
{
    NVIDIA = 0x10DE,
    AMD    = 0x1002,
    Intel  = 0x8086
};

inline const char* GetRHIVendorString(RHI_VENDOR Vendor)
{
    switch (Vendor)
    {
        case RHI_VENDOR::NVIDIA:
            return "NVIDIA";
        case RHI_VENDOR::AMD:
            return "AMD";
        case RHI_VENDOR::Intel:
            return "Intel";
    }
    return "<unknown>";
}

enum class RHI_SHADER_MODEL
{
    ShaderModel_6_5,
    ShaderModel_6_6
};

enum class RHI_SHADER_TYPE
{
    Vertex,
    Hull,
    Domain,
    Geometry,
    Pixel,
    Compute,
    Amplification,
    Mesh
};

enum class RHI_PIPELINE_STATE_TYPE
{
    Graphics,
    Compute,
};

enum class RHI_PIPELINE_STATE_SUBOBJECT_TYPE
{
    RootSignature,
    InputLayout,
    VS,
    PS,
    DS,
    HS,
    GS,
    CS,
    AS,
    MS,
    BlendState,
    RasterizerState,
    DepthStencilState,
    RenderTargetState,
    SampleState,
    PrimitiveTopology,

    NumTypes
};

inline const char* GetRHIPipelineStateSubobjectTypeString(RHI_PIPELINE_STATE_SUBOBJECT_TYPE Type)
{
    switch (Type)
    {
        case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::RootSignature:
            return "Root Signature";
        case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::InputLayout:
            return "Input Layout";
        case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::VS:
            return "Vertex Shader";
        case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::PS:
            return "Pixel Shader";
        case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::DS:
            return "Domain Shader";
        case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::HS:
            return "Hull Shader";
        case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::GS:
            return "Geometry Shader";
        case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::CS:
            return "Compute Shader";
        case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::AS:
            return "Amplification Shader";
        case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::MS:
            return "Mesh Shader";
        case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::BlendState:
            return "Blend State";
        case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::RasterizerState:
            return "Rasterizer State";
        case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::DepthStencilState:
            return "Depth Stencil State";
        case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::RenderTargetState:
            return "Render Target State";
        case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::PrimitiveTopology:
            return "Primitive Topology";
    }
    return "<unknown>";
}

enum class RHI_PRIMITIVE_TOPOLOGY
{
    Undefined,
    Point,
    Line,
    Triangle,
    Patch
};

enum class RHI_COMPARISON_FUNC
{
    Never,        // Comparison always fails
    Less,         // Passes if source is less than the destination
    Equal,        // Passes if source is equal to the destination
    LessEqual,    // Passes if source is less than or equal to the destination
    Greater,      // Passes if source is greater than to the destination
    NotEqual,     // Passes if source is not equal to the destination
    GreaterEqual, // Passes if source is greater than or equal to the destination
    Always        // Comparison always succeeds
};

enum class RHI_SAMPLER_FILTER
{
    Point,
    Linear,
    Anisotropic
};

enum class RHI_SAMPLER_ADDRESS_MODE
{
    Wrap,
    Mirror,
    Clamp,
    Border,
};

enum class RHI_BLEND_OP
{
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max
};

enum class RHI_FACTOR
{
    Zero,                // (0, 0, 0, 0)
    One,                 // (1, 1, 1, 1)
    SrcColor,            // The pixel-shader output color
    OneMinusSrcColor,    // One minus the pixel-shader output color
    DstColor,            // The render-target color
    OneMinusDstColor,    // One minus the render-target color
    SrcAlpha,            // The pixel-shader output alpha value
    OneMinusSrcAlpha,    // One minus the pixel-shader output alpha value
    DstAlpha,            // The render-target alpha value
    OneMinusDstAlpha,    // One minus the render-target alpha value
    BlendFactor,         // Constant color, set using CommandList
    OneMinusBlendFactor, // One minus constant color, set using CommandList
    SrcAlphaSaturate,    // (f, f, f, 1), where f = min(pixel shader output alpha, 1 - render-target pixel alpha)
    Src1Color,           // Fragment-shader output color 1
    OneMinusSrc1Color,   // One minus pixel-shader output color 1
    Src1Alpha,           // Fragment-shader output alpha 1
    OneMinusSrc1Alpha    // One minus pixel-shader output alpha 1
};

struct RHIRenderTargetBlendDesc
{
    bool         BlendEnable   = false;
    RHI_FACTOR   SrcBlendRgb   = RHI_FACTOR::One;
    RHI_FACTOR   DstBlendRgb   = RHI_FACTOR::Zero;
    RHI_BLEND_OP BlendOpRgb    = RHI_BLEND_OP::Add;
    RHI_FACTOR   SrcBlendAlpha = RHI_FACTOR::One;
    RHI_FACTOR   DstBlendAlpha = RHI_FACTOR::Zero;
    RHI_BLEND_OP BlendOpAlpha  = RHI_BLEND_OP::Add;

    struct WriteMask
    {
        bool R = true;
        bool G = true;
        bool B = true;
        bool A = true;
    } WriteMask;
};

enum class RHI_FILL_MODE
{
    Wireframe, // Draw lines connecting the vertices.
    Solid      // Fill the triangles formed by the vertices
};

enum class RHI_CULL_MODE
{
    None,  // Always draw all triangles
    Front, // Do not draw triangles that are front-facing
    Back   // Do not draw triangles that are back-facing
};

enum class RHI_STENCIL_OP
{
    Keep,             // Keep the stencil value
    Zero,             // Set the stencil value to zero
    Replace,          // Replace the stencil value with the reference value
    IncreaseSaturate, // Increase the stencil value by one, clamp if necessary
    DecreaseSaturate, // Decrease the stencil value by one, clamp if necessary
    Invert,           // Invert the stencil data (bitwise not)
    Increase,         // Increase the stencil value by one, wrap if necessary
    Decrease          // Decrease the stencil value by one, wrap if necessary
};

struct RHIDepthStencilOpDesc
{
    RHI_STENCIL_OP      StencilFailOp      = RHI_STENCIL_OP::Keep;
    RHI_STENCIL_OP      StencilDepthFailOp = RHI_STENCIL_OP::Keep;
    RHI_STENCIL_OP      StencilPassOp      = RHI_STENCIL_OP::Keep;
    RHI_COMPARISON_FUNC StencilFunc        = RHI_COMPARISON_FUNC::Always;
};

struct RHIBlendState
{
    bool                     AlphaToCoverageEnable  = false;
    bool                     IndependentBlendEnable = false;
    RHIRenderTargetBlendDesc RenderTargets[8];
};

struct RHIRasterizerState
{
    RHI_FILL_MODE FillMode              = RHI_FILL_MODE::Solid;
    RHI_CULL_MODE CullMode              = RHI_CULL_MODE::Back;
    bool          FrontCounterClockwise = TRUE;
    int           DepthBias             = D3D12_DEFAULT_DEPTH_BIAS;
    float         DepthBiasClamp        = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    float         SlopeScaledDepthBias  = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    bool          DepthClipEnable       = TRUE;
    bool          MultisampleEnable     = FALSE;
    bool          AntialiasedLineEnable = FALSE;
    unsigned int  ForcedSampleCount     = 0;
    bool          ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
};

struct RHIDepthStencilState
{
    // Default states
    // https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_depth_stencil_desc#remarks
    bool                  DepthEnable      = true;
    bool                  DepthWrite       = true;
    RHI_COMPARISON_FUNC   DepthFunc        = RHI_COMPARISON_FUNC::Less;
    bool                  StencilEnable    = false;
    std::byte             StencilReadMask  = std::byte {0xff};
    std::byte             StencilWriteMask = std::byte {0xff};
    RHIDepthStencilOpDesc FrontFace;
    RHIDepthStencilOpDesc BackFace;
};

struct RHIRenderTargetState
{
    DXGI_FORMAT   RTFormats[8]     = {DXGI_FORMAT_UNKNOWN};
    std::uint32_t NumRenderTargets = 0;
    DXGI_FORMAT   DSFormat         = DXGI_FORMAT_UNKNOWN;
};

enum class RHIResourceState : uint64_t
{
    RHI_RESOURCE_STATE_COMMON	= 0,
    RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER	= 0x1,
    RHI_RESOURCE_STATE_INDEX_BUFFER	= 0x2,
    RHI_RESOURCE_STATE_RENDER_TARGET	= 0x4,
    RHI_RESOURCE_STATE_UNORDERED_ACCESS	= 0x8,
    RHI_RESOURCE_STATE_DEPTH_WRITE	= 0x10,
    RHI_RESOURCE_STATE_DEPTH_READ	= 0x20,
    RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE	= 0x40,
    RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE	= 0x80,
    RHI_RESOURCE_STATE_STREAM_OUT	= 0x100,
    RHI_RESOURCE_STATE_INDIRECT_ARGUMENT	= 0x200,
    RHI_RESOURCE_STATE_COPY_DEST	= 0x400,
    RHI_RESOURCE_STATE_COPY_SOURCE	= 0x800,
    RHI_RESOURCE_STATE_RESOLVE_DEST	= 0x1000,
    RHI_RESOURCE_STATE_RESOLVE_SOURCE	= 0x2000,
    RHI_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE	= 0x400000,
    RHI_RESOURCE_STATE_SHADING_RATE_SOURCE	= 0x1000000,
    RHI_RESOURCE_STATE_GENERIC_READ	= ( ( ( ( ( 0x1 | 0x2 )  | 0x40 )  | 0x80 )  | 0x200 )  | 0x800 ) ,
    RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE	= ( 0x40 | 0x80 ) ,
    RHI_RESOURCE_STATE_PRESENT	= 0,
    RHI_RESOURCE_STATE_PREDICATION	= 0x200,
    RHI_RESOURCE_STATE_NONE = 0xffffffffffffffff,
};
inline bool operator==(RHIResourceState l, RHIResourceState r) { return (uint64_t)l == (uint64_t)r; }
inline bool operator!=(RHIResourceState l, RHIResourceState r) { return (uint64_t)l != (uint64_t)r; }


struct RHISampleState
{
    std::uint32_t Count   = 1;
    std::uint32_t Quality = 0;
};

struct RHIViewport
{
    float TopLeftX;
    float TopLeftY;
    float Width;
    float Height;
    float MinDepth;
    float MaxDepth;
};

struct RHIRect
{
    long Left;
    long Top;
    long Right;
    long Bottom;
};

template<typename TDesc, RHI_PIPELINE_STATE_SUBOBJECT_TYPE TType>
class alignas(void*) PipelineStateStreamSubobject
{
public:
    PipelineStateStreamSubobject() noexcept : Type(TType), Desc(TDesc()) {}
    PipelineStateStreamSubobject(const TDesc& Desc) noexcept : Type(TType), Desc(Desc) {}
    PipelineStateStreamSubobject& operator=(const TDesc& Desc) noexcept
    {
        this->Type = TType;
        this->Desc = Desc;
        return *this;
    }
                 operator const TDesc&() const noexcept { return Desc; }
                 operator TDesc&() noexcept { return Desc; }
    TDesc*       operator&() noexcept { return &Desc; }
    const TDesc* operator&() const noexcept { return &Desc; }

    TDesc& operator->() noexcept { return Desc; }

private:
    RHI_PIPELINE_STATE_SUBOBJECT_TYPE Type;
    TDesc                             Desc;
};

namespace RHI
{
    class D3D12RootSignature;
    class D3D12InputLayout;
} // namespace RHI
class Shader;

using PipelineStateStreamRootSignature = PipelineStateStreamSubobject<RHI::D3D12RootSignature*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::RootSignature>;
using PipelineStateStreamInputLayout = PipelineStateStreamSubobject<RHI::D3D12InputLayout*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::InputLayout>;
using PipelineStateStreamVS = PipelineStateStreamSubobject<Shader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::VS>;
using PipelineStateStreamPS = PipelineStateStreamSubobject<Shader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::PS>;
using PipelineStateStreamDS = PipelineStateStreamSubobject<Shader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::DS>;
using PipelineStateStreamHS = PipelineStateStreamSubobject<Shader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::HS>;
using PipelineStateStreamGS = PipelineStateStreamSubobject<Shader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::GS>;
using PipelineStateStreamCS = PipelineStateStreamSubobject<Shader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::CS>;
using PipelineStateStreamAS = PipelineStateStreamSubobject<Shader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::AS>;
using PipelineStateStreamMS = PipelineStateStreamSubobject<Shader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::MS>;
using PipelineStateStreamBlendState = PipelineStateStreamSubobject<RHIBlendState, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::BlendState>;
using PipelineStateStreamRasterizerState = PipelineStateStreamSubobject<RHIRasterizerState, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::RasterizerState>;
using PipelineStateStreamDepthStencilState = PipelineStateStreamSubobject<RHIDepthStencilState, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::DepthStencilState>;
using PipelineStateStreamRenderTargetState = PipelineStateStreamSubobject<RHIRenderTargetState, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::RenderTargetState>;
using PipelineStateStreamSampleState = PipelineStateStreamSubobject<RHISampleState, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::SampleState>;
using PipelineStateStreamPrimitiveTopology = PipelineStateStreamSubobject<RHI_PRIMITIVE_TOPOLOGY, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::PrimitiveTopology>;

class IPipelineParserCallbacks
{
public:
    virtual ~IPipelineParserCallbacks() = default;

    // Subobject Callbacks
    virtual void RootSignatureCb(RHI::D3D12RootSignature*) {}
    virtual void InputLayoutCb(RHI::D3D12InputLayout*) {}
    virtual void VSCb(Shader*) {}
    virtual void PSCb(Shader*) {}
    virtual void DSCb(Shader*) {}
    virtual void HSCb(Shader*) {}
    virtual void GSCb(Shader*) {}
    virtual void CSCb(Shader*) {}
    virtual void ASCb(Shader*) {}
    virtual void MSCb(Shader*) {}
    virtual void BlendStateCb(const RHIBlendState&) {}
    virtual void RasterizerStateCb(const RHIRasterizerState&) {}
    virtual void DepthStencilStateCb(const RHIDepthStencilState&) {}
    virtual void RenderTargetStateCb(const RHIRenderTargetState&) {}
    virtual void SampleStateCb(const RHISampleState&) {}
    virtual void PrimitiveTopologyTypeCb(RHI_PRIMITIVE_TOPOLOGY) {}

    // Error Callbacks
    virtual void ErrorBadInputParameter(size_t /*ParameterIndex*/) {}
    virtual void ErrorDuplicateSubobject(RHI_PIPELINE_STATE_SUBOBJECT_TYPE /*DuplicateType*/) {}
    virtual void ErrorUnknownSubobject(size_t /*UnknownTypeValue*/) {}
};

struct PipelineStateStreamDesc
{
    size_t SizeInBytes;
    void*  pPipelineStateSubobjectStream;
};

void RHIParsePipelineStream(const PipelineStateStreamDesc& Desc, IPipelineParserCallbacks* Callbacks);




























