#pragma once
#include "d3d12_core.h"

namespace RHI
{
    // The maximum size of a root signature is 64 DWORDs.
    // This maximum size is chosen to prevent abuse of the root signature as a way of storing bulk data. Each entry in
    // the root signature has a cost towards this 64 DWORD limit: Descriptor tables cost 1 DWORD each. Root constants
    // cost 1 DWORD each, since they are 32-bit values. Root descriptors (64-bit GPU virtual addresses) cost 2 DWORDs
    // each. Static samplers do not have any cost in the size of the root signature. Use a small a root signature as
    // necessary, though balance this with the flexibility of a larger root signature. Arrange parameters in a large
    // root signature so that the parameters most likely to change often, or if low access latency for a given parameter
    // is important, occur first. If convenient, use root constants or root constant buffer views over putting constant
    // buffer views in a descriptor heap.

    // Describes ranges of descriptors that might have offsets relative to the initial range descriptor
    // I use this to emulate bindless resource, SM6.6 provides global variables for DescriptorHeaps
    // that you can directly index into

    // clang-format off
    class D3D12DescriptorTable
    {
    public:
        explicit D3D12DescriptorTable(size_t NumRanges) { DescriptorRanges.reserve(NumRanges); }

        template<UINT BaseShaderRegister, UINT RegisterSpace>
        D3D12DescriptorTable& AddSRVRange(UINT NumDescriptors, D3D12_DESCRIPTOR_RANGE_FLAGS Flags, UINT OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
        {
            return AddDescriptorRange<BaseShaderRegister, RegisterSpace>(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, NumDescriptors, Flags, OffsetInDescriptorsFromTableStart);
        }

        template<UINT BaseShaderRegister, UINT RegisterSpace>
        D3D12DescriptorTable& AddUAVRange(UINT NumDescriptors, D3D12_DESCRIPTOR_RANGE_FLAGS Flags, UINT OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
        {
            return AddDescriptorRange<BaseShaderRegister, RegisterSpace>(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, NumDescriptors, Flags, OffsetInDescriptorsFromTableStart);
        }

        template<UINT BaseShaderRegister, UINT RegisterSpace>
        D3D12DescriptorTable& AddCBVRange(UINT NumDescriptors, D3D12_DESCRIPTOR_RANGE_FLAGS Flags, UINT OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
        {
            return AddDescriptorRange<BaseShaderRegister, RegisterSpace>(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, NumDescriptors, Flags, OffsetInDescriptorsFromTableStart);
        }

        template<UINT BaseShaderRegister, UINT RegisterSpace>
        D3D12DescriptorTable&
        AddSamplerRange(UINT NumDescriptors, D3D12_DESCRIPTOR_RANGE_FLAGS Flags, UINT OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
        {
            return AddDescriptorRange<BaseShaderRegister, RegisterSpace>(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, NumDescriptors, Flags, OffsetInDescriptorsFromTableStart);
        }

        template<UINT BaseShaderRegister, UINT RegisterSpace>
        D3D12DescriptorTable& AddDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE Type, UINT NumDescriptors, D3D12_DESCRIPTOR_RANGE_FLAGS Flags, UINT OffsetInDescriptorsFromTableStart)
        {
            CD3DX12_DESCRIPTOR_RANGE1& Range = DescriptorRanges.emplace_back();
            Range.Init(Type, NumDescriptors, BaseShaderRegister, RegisterSpace, Flags, OffsetInDescriptorsFromTableStart);
            return *this;
        }

        UINT                           size() const noexcept { return static_cast<UINT>(DescriptorRanges.size()); }
        const D3D12_DESCRIPTOR_RANGE1* data() const noexcept { return DescriptorRanges.data(); }

    private:
        std::vector<CD3DX12_DESCRIPTOR_RANGE1> DescriptorRanges;
    };
    // clang-format on

    class RootSignatureDesc
    {
    public:
        RootSignatureDesc();

        RootSignatureDesc& AddDescriptorTable(const D3D12DescriptorTable& DescriptorTable);

        template<UINT ShaderRegister, UINT RegisterSpace, typename T>
        RootSignatureDesc& Add32BitConstants()
        {
            static_assert(sizeof(T) % 4 == 0);
            CD3DX12_ROOT_PARAMETER1 Parameter = {};
            Parameter.InitAsConstants(sizeof(T) / 4, ShaderRegister, RegisterSpace);
            return AddParameter(Parameter);
        }

        template<UINT ShaderRegister, UINT RegisterSpace>
        RootSignatureDesc& Add32BitConstants(UINT Num32BitValues)
        {
            CD3DX12_ROOT_PARAMETER1 Parameter = {};
            Parameter.InitAsConstants(Num32BitValues, ShaderRegister, RegisterSpace);
            return AddParameter(Parameter);
        }

        template<UINT ShaderRegister, UINT RegisterSpace>
        RootSignatureDesc& AddConstantBufferView(D3D12_ROOT_DESCRIPTOR_FLAGS Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE)
        {
            CD3DX12_ROOT_PARAMETER1 Parameter = {};
            Parameter.InitAsConstantBufferView(ShaderRegister, RegisterSpace, Flags);
            return AddParameter(Parameter);
        }

        template<UINT ShaderRegister, UINT RegisterSpace>
        RootSignatureDesc& AddShaderResourceView(D3D12_ROOT_DESCRIPTOR_FLAGS Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE)
        {
            CD3DX12_ROOT_PARAMETER1 Parameter = {};
            Parameter.InitAsShaderResourceView(ShaderRegister, RegisterSpace, Flags);
            return AddParameter(Parameter);
        }

        template<UINT ShaderRegister, UINT RegisterSpace>
        RootSignatureDesc& AddUnorderedAccessView(D3D12_ROOT_DESCRIPTOR_FLAGS Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE)
        {
            CD3DX12_ROOT_PARAMETER1 Parameter = {};
            Parameter.InitAsUnorderedAccessView(ShaderRegister, RegisterSpace, Flags);
            return AddParameter(Parameter);
        }

        template<UINT ShaderRegister, UINT RegisterSpace>
        RootSignatureDesc& AddUnorderedAccessViewWithCounter(D3D12_DESCRIPTOR_RANGE_FLAGS Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE)
        {
            D3D12DescriptorTable DescriptorTable(1);
            DescriptorTable.AddUAVRange<ShaderRegister, RegisterSpace>(1, Flags);
            return AddDescriptorTable(DescriptorTable);
        }

        template<UINT ShaderRegister, UINT RegisterSpace>
        RootSignatureDesc& AddRaytracingAccelerationStructure(D3D12_DESCRIPTOR_RANGE_FLAGS Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE)
        {
            D3D12DescriptorTable DescriptorTable(1);
            DescriptorTable.AddSRVRange<ShaderRegister, RegisterSpace>(1, Flags);
            return AddDescriptorTable(DescriptorTable);
        }

        template<UINT ShaderRegister, UINT RegisterSpace>
        RootSignatureDesc& AddStaticSampler(D3D12_FILTER               Filter,
                                            D3D12_TEXTURE_ADDRESS_MODE AddressUVW,
                                            UINT                       MaxAnisotropy,
                                            D3D12_COMPARISON_FUNC      ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
                                            D3D12_STATIC_BORDER_COLOR  BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE)
        {
            CD3DX12_STATIC_SAMPLER_DESC& Desc = StaticSamplers.emplace_back();
            Desc.Init(ShaderRegister,
                      Filter,
                      AddressUVW,
                      AddressUVW,
                      AddressUVW,
                      0.0f,
                      MaxAnisotropy,
                      ComparisonFunc,
                      BorderColor);
            Desc.RegisterSpace = RegisterSpace;
            return *this;
        }

        template<UINT ShaderRegister, UINT RegisterSpace>
        RootSignatureDesc& AddStaticSampler(const D3D12_SAMPLER_DESC& NonStaticSamplerDesc,
                                            D3D12_SHADER_VISIBILITY   Visibility = D3D12_SHADER_VISIBILITY_ALL)
        {
            CD3DX12_STATIC_SAMPLER_DESC& Desc = StaticSamplers.emplace_back();
            Desc.Filter                       = NonStaticSamplerDesc.Filter;
            Desc.AddressU                     = NonStaticSamplerDesc.AddressU;
            Desc.AddressV                     = NonStaticSamplerDesc.AddressV;
            Desc.AddressW                     = NonStaticSamplerDesc.AddressW;
            Desc.MipLODBias                   = NonStaticSamplerDesc.MipLODBias;
            Desc.MaxAnisotropy                = NonStaticSamplerDesc.MaxAnisotropy;
            Desc.ComparisonFunc               = NonStaticSamplerDesc.ComparisonFunc;
            Desc.BorderColor                  = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
            Desc.MinLOD                       = NonStaticSamplerDesc.MinLOD;
            Desc.MaxLOD                       = NonStaticSamplerDesc.MaxLOD;
            Desc.ShaderRegister               = ShaderRegister;
            Desc.RegisterSpace                = RegisterSpace;
            Desc.ShaderVisibility             = Visibility;

            if (Desc.AddressU == D3D12_TEXTURE_ADDRESS_MODE_BORDER ||
                Desc.AddressV == D3D12_TEXTURE_ADDRESS_MODE_BORDER ||
                Desc.AddressW == D3D12_TEXTURE_ADDRESS_MODE_BORDER)
            {
                ASSERT(
                    // Transparent Black
                    NonStaticSamplerDesc.BorderColor[0] == 0.0f && NonStaticSamplerDesc.BorderColor[1] == 0.0f &&
                        NonStaticSamplerDesc.BorderColor[2] == 0.0f && NonStaticSamplerDesc.BorderColor[3] == 0.0f ||
                    // Opaque Black
                    NonStaticSamplerDesc.BorderColor[0] == 0.0f && NonStaticSamplerDesc.BorderColor[1] == 0.0f &&
                        NonStaticSamplerDesc.BorderColor[2] == 0.0f && NonStaticSamplerDesc.BorderColor[3] == 1.0f ||
                    // Opaque White
                    NonStaticSamplerDesc.BorderColor[0] == 1.0f && NonStaticSamplerDesc.BorderColor[1] == 1.0f &&
                        NonStaticSamplerDesc.BorderColor[2] == 1.0f && NonStaticSamplerDesc.BorderColor[3] == 1.0f);

                if (NonStaticSamplerDesc.BorderColor[3] == 1.0f)
                {
                    if (NonStaticSamplerDesc.BorderColor[0] == 1.0f)
                        Desc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
                    else
                        Desc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
                }
                else
                    Desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            }
            return *this;
        }

        RootSignatureDesc& AllowInputLayout() noexcept;
        RootSignatureDesc& DenyVSAccess() noexcept;
        RootSignatureDesc& DenyHSAccess() noexcept;
        RootSignatureDesc& DenyDSAccess() noexcept;
        RootSignatureDesc& DenyTessellationShaderAccess() noexcept;
        RootSignatureDesc& DenyGSAccess() noexcept;
        RootSignatureDesc& DenyPSAccess() noexcept;
        RootSignatureDesc& AsLocalRootSignature() noexcept;
        RootSignatureDesc& DenyASAccess() noexcept;
        RootSignatureDesc& DenyMSAccess() noexcept;
        RootSignatureDesc& AllowResourceDescriptorHeapIndexing() noexcept;
        RootSignatureDesc& AllowSampleDescriptorHeapIndexing() noexcept;

        [[nodiscard]] bool IsLocal() const noexcept;

        D3D12_ROOT_SIGNATURE_DESC1 Build() noexcept;

    private:
        RootSignatureDesc& AddParameter(D3D12_ROOT_PARAMETER1 Parameter);

    private:
        std::vector<CD3DX12_ROOT_PARAMETER1>     Parameters;
        std::vector<CD3DX12_STATIC_SAMPLER_DESC> StaticSamplers;
        D3D12_ROOT_SIGNATURE_FLAGS               Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

        std::vector<UINT>                 DescriptorTableIndices;
        std::vector<D3D12DescriptorTable> DescriptorTables;
    };

    class D3D12RootSignature final : public D3D12DeviceChild
    {
    public:
        D3D12RootSignature() noexcept = default;
        explicit D3D12RootSignature(D3D12Device* Parent, RootSignatureDesc& Desc);

        D3D12RootSignature(D3D12RootSignature&&) noexcept = default;
        D3D12RootSignature& operator=(D3D12RootSignature&&) noexcept = default;

        D3D12RootSignature(const D3D12RootSignature&) = delete;
        D3D12RootSignature& operator=(const D3D12RootSignature&) = delete;

        [[nodiscard]] ID3D12RootSignature* GetApiHandle() const noexcept { return m_RootSignature.Get(); }
        [[nodiscard]] UINT                 GetNumParameters() const noexcept { return m_NumParameters; }
        [[nodiscard]] UINT                 GetNumStaticSamplers() const noexcept { return m_NumStaticSamplers; }

        [[nodiscard]] UINT32 GetDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE Type) const noexcept;

        [[nodiscard]] UINT GetDescriptorTableSize(UINT RootParameterIndex) const noexcept;

    //private:
    //    // Pre SM6.6 bindless root parameter setup
    //    static void AddBindlessParameters(RootSignatureDesc& Desc);

    private:
        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;

        UINT m_NumParameters = 0;
        UINT m_NumStaticSamplers = 0;
        UINT32 m_DescriptorTableBitMask;  // One bit is set for root parameters that are non-sampler descriptor tables
        UINT32 m_SamplerTableBitMask;     // One bit is set for root parameters that are sampler descriptor tables
        UINT32 m_DescriptorTableSize[16]; // Non-sampler descriptor tables need to know their descriptor count


        /*
        // Need to know the number of descriptors per descriptor table
        UINT m_NumDescriptorsPerTable[MOYU_RHI_D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT] = {};

        // A bit mask that represents the root parameter indices that are
        // descriptor tables for CBV, UAV, and SRV
        std::bitset<MOYU_RHI_D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT> m_ResourceDescriptorTableBitMask;
        // A bit mask that represents the root parameter indices that are
        // descriptor tables for Samplers
        std::bitset<MOYU_RHI_D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT> m_SamplerTableBitMask;
        */
    };
} // namespace RHI
