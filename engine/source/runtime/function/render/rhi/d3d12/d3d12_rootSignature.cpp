#include "d3d12_rootSignature.h"
#include "d3d12_device.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/global/global_context.h"

namespace RHI
{
    RootSignatureDesc::RootSignatureDesc()
    {
        // Worst case number of root parameters is 64
        // Preallocate all possible parameters to avoid cost for push back
        Parameters.reserve(D3D12_MAX_ROOT_COST);
        DescriptorTableIndices.reserve(MOYU_RHI_D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT);
        DescriptorTables.reserve(MOYU_RHI_D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT);
    }

    RootSignatureDesc& RootSignatureDesc::AddDescriptorTable(const D3D12DescriptorTable& DescriptorTable)
    {
        // The descriptor table descriptor ranges require a pointer to the descriptor ranges. Since new
        // ranges can be dynamically added in the vector, we separately store the index of the range set.
        // The actual address will be solved when generating the actual root signature
        CD3DX12_ROOT_PARAMETER1& Parameter = Parameters.emplace_back();
        Parameter.InitAsDescriptorTable(DescriptorTable.size(), nullptr);
        DescriptorTableIndices.push_back(static_cast<UINT>(DescriptorTables.size()));
        DescriptorTables.push_back(DescriptorTable);
        return *this;
    }

	RootSignatureDesc& RootSignatureDesc::AllowInputLayout() noexcept
    {
        Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        return *this;
    }

	RootSignatureDesc& RootSignatureDesc::DenyVSAccess() noexcept
    {
        Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
        return *this;
    }

	RootSignatureDesc& RootSignatureDesc::DenyHSAccess() noexcept
    {
        Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
        return *this;
    }

	RootSignatureDesc& RootSignatureDesc::DenyDSAccess() noexcept
    {
        Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
        return *this;
    }

	RootSignatureDesc& RootSignatureDesc::DenyTessellationShaderAccess() noexcept
    {
        DenyHSAccess();
        DenyDSAccess();
        return *this;
    }

	RootSignatureDesc& RootSignatureDesc::DenyGSAccess() noexcept
    {
        Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
        return *this;
    }

    RootSignatureDesc& RootSignatureDesc::DenyPSAccess() noexcept
    {
        Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
        return *this;
    }

	RootSignatureDesc& RootSignatureDesc::AsLocalRootSignature() noexcept
    {
        Flags |= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
        return *this;
    }

    RootSignatureDesc& RootSignatureDesc::DenyASAccess() noexcept
    {
        Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
        return *this;
    }

    RootSignatureDesc& RootSignatureDesc::DenyMSAccess() noexcept
    {
        Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;
        return *this;
    }

    RootSignatureDesc& RootSignatureDesc::AllowResourceDescriptorHeapIndexing() noexcept
    {
        Flags |= D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
        return *this;
    }

    RootSignatureDesc& RootSignatureDesc::AllowSampleDescriptorHeapIndexing() noexcept
    {
        Flags |= D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;
        return *this;
    }

    bool RootSignatureDesc::IsLocal() const noexcept { return Flags & D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE; }

	D3D12_ROOT_SIGNATURE_DESC1 RootSignatureDesc::Build() noexcept
    {
        // Go through all the parameters, and set the actual addresses of the heap range descriptors based
        // on their indices in the range indices vector
        for (size_t i = 0; i < Parameters.size(); ++i)
        {
            if (Parameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
            {
                Parameters[i].DescriptorTable.NumDescriptorRanges = DescriptorTables[DescriptorTableIndices[i]].size();
                Parameters[i].DescriptorTable.pDescriptorRanges   = DescriptorTables[DescriptorTableIndices[i]].data();
            }
        }

        D3D12_ROOT_SIGNATURE_DESC1 Desc = {static_cast<UINT>(Parameters.size()),
                                           Parameters.data(),
                                           static_cast<UINT>(StaticSamplers.size()),
                                           StaticSamplers.data(),
                                           Flags};
        return Desc;
    }

	RootSignatureDesc& RootSignatureDesc::AddParameter(D3D12_ROOT_PARAMETER1 Parameter)
    {
        Parameters.emplace_back(Parameter);
        DescriptorTableIndices.emplace_back(0xDEADBEEF);
        return *this;
    }

    D3D12RootSignature::D3D12RootSignature(D3D12Device* Parent, RootSignatureDesc& Desc) : D3D12DeviceChild(Parent)
    {
        if (!Desc.IsLocal())
        {
            //// If a root signature is local we don't add bindless descriptor table because it will conflict with global
            //// root signature
            //AddBindlessParameters(Desc);
        }

        D3D12_VERSIONED_ROOT_SIGNATURE_DESC ApiDesc;
        ApiDesc.Version  = D3D_ROOT_SIGNATURE_VERSION_1_1;
        ApiDesc.Desc_1_1 = Desc.Build();

        m_NumParameters = ApiDesc.Desc_1_1.NumParameters;
        m_NumStaticSamplers = ApiDesc.Desc_1_1.NumStaticSamplers;

        // Serialize the root signature
        Microsoft::WRL::ComPtr<ID3DBlob> SerializedRootSignatureBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> ErrorBlob;
        HRESULT Result = D3D12SerializeVersionedRootSignature(&ApiDesc, &SerializedRootSignatureBlob, &ErrorBlob);
        if (FAILED(Result))
        {
            assert(ErrorBlob);
            LOG_ERROR("{}", static_cast<const char*>(ErrorBlob->GetBufferPointer()));
        }
        VERIFY_D3D12_API(Result);

        // Create the root signature
        VERIFY_D3D12_API(Parent->GetD3D12Device()->CreateRootSignature(Parent->GetAllNodeMask(),
                                                                       SerializedRootSignatureBlob->GetBufferPointer(),
                                                                       SerializedRootSignatureBlob->GetBufferSize(),
                                                                       IID_PPV_ARGS(&m_RootSignature)));

        m_DescriptorTableBitMask = 0;
        m_SamplerTableBitMask    = 0;

        memset(m_DescriptorTableSize, 0, sizeof(m_DescriptorTableSize));

        for (UINT i = 0; i < ApiDesc.Desc_1_1.NumParameters; ++i)
        {
            const D3D12_ROOT_PARAMETER1& RootParameter = ApiDesc.Desc_1_1.pParameters[i];
            if (RootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
            {
                const D3D12_ROOT_DESCRIPTOR_TABLE1& DescriptorTable1 = RootParameter.DescriptorTable;

                // Don't care about the rest, just the first range is enough
                switch (DescriptorTable1.pDescriptorRanges[0].RangeType)
                {
                    case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
                    case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
                    case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
                        m_DescriptorTableBitMask |= (1 << i); 
                        break;
                    case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
                        m_SamplerTableBitMask |= (1 << i); 
                        break;
                }

                // Calculate total number of descriptors in the descriptor table.
                for (UINT j = 0; j < DescriptorTable1.NumDescriptorRanges; ++j)
                {
                    m_DescriptorTableSize[i] += DescriptorTable1.pDescriptorRanges[j].NumDescriptors;
                }
            }
        }
    }

	uint32_t D3D12RootSignature::GetDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE Type) const noexcept
    {
        switch (Type)
        {
            case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
                return m_DescriptorTableBitMask;
            case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
                return m_SamplerTableBitMask;
            default:
                return {};
        }
    }

    UINT D3D12RootSignature::GetDescriptorTableSize(UINT RootParameterIndex) const noexcept
    {
        ASSERT(RootParameterIndex < MOYU_RHI_D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT);
        return m_DescriptorTableSize[RootParameterIndex];
    }

    //// SM6.6�Ѿ�֧����Dynamic Resources��������Ȼ��ΪLocalRootSignature׼���ģ����Ƕ���Graphics��Compute�Ժ�Ҳ������Ҫ��
    //// HLSL Dynamic Resources
    //// https://microsoft.github.io/DirectX-Specs/d3d/HLSL_ShaderModel6_6.html
    //// https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_DynamicResources.html
    //void D3D12RootSignature::AddBindlessParameters(RootSignatureDesc& Desc)
    //{
    //    // TODO: Maybe consider this as a fall back options when SM6.6 dynamic resource binding is integrated
    //    /* Descriptor Tables */
    //    constexpr D3D12_DESCRIPTOR_RANGE_FLAGS DescriptorDataVolatile =
    //        D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
    //    constexpr D3D12_DESCRIPTOR_RANGE_FLAGS DescriptorVolatile = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
    //    Desc.AddDescriptorTable(
    //            // ShaderResource
    //            D3D12DescriptorTable(4)
    //                .AddSRVRange<0, 100>(UINT_MAX, DescriptorDataVolatile, 0)  // g_ByteAddressBufferTable
    //                .AddSRVRange<0, 101>(UINT_MAX, DescriptorDataVolatile, 0)  // g_Texture2DTable
    //                .AddSRVRange<0, 102>(UINT_MAX, DescriptorDataVolatile, 0)  // g_Texture2DArrayTable
    //                .AddSRVRange<0, 103>(UINT_MAX, DescriptorDataVolatile, 0)) // g_TextureCubeTable
    //        .AddDescriptorTable(
    //            // UnorderedAccess
    //            D3D12DescriptorTable(2)
    //                .AddUAVRange<0, 100>(UINT_MAX, DescriptorDataVolatile, 0)  // g_RWTexture2DTable
    //                .AddUAVRange<0, 101>(UINT_MAX, DescriptorDataVolatile, 0)) // g_RWTexture2DArrayTable
    //        .AddDescriptorTable(
    //            // Sampler
    //            D3D12DescriptorTable(1).AddSamplerRange<0, 100>(UINT_MAX, DescriptorVolatile, 0)); // g_SamplerTable
    //    constexpr D3D12_FILTER               PointFilter  = D3D12_FILTER_MIN_MAG_MIP_POINT;
    //    constexpr D3D12_FILTER               LinearFilter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    //    constexpr D3D12_FILTER               Anisotropic  = D3D12_FILTER_ANISOTROPIC;
    //    constexpr D3D12_TEXTURE_ADDRESS_MODE Wrap         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    //    constexpr D3D12_TEXTURE_ADDRESS_MODE Clamp        = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    //    constexpr D3D12_TEXTURE_ADDRESS_MODE Border       = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    //    Desc.AddSampler<0, 101>(PointFilter, Wrap, 16)  // g_SamplerPointWrap
    //        .AddSampler<1, 101>(PointFilter, Clamp, 16) // g_SamplerPointClamp
    //        .AddSampler<2, 101>(LinearFilter, Wrap, 16)  // g_SamplerLinearWrap
    //        .AddSampler<3, 101>(LinearFilter, Clamp, 16) // g_SamplerLinearClamp
    //        .AddSampler<4, 101>(LinearFilter,
    //                            Border,
    //                            16,
    //                            D3D12_COMPARISON_FUNC_LESS_EQUAL,
    //                            D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK) // g_SamplerLinearBorder
    //        .AddSampler<5, 101>(Anisotropic, Wrap, 16)   // g_SamplerAnisotropicWrap
    //        .AddSampler<6, 101>(Anisotropic, Clamp, 16); // g_SamplerAnisotropicClamp
    //}
}
