#include "render_common.h"
#include "runtime/resource/res_type/components/material.h"

namespace MoYu
{
    SceneImage DefaultSceneImageWhite{ true, false, 1, "asset/texture/default/white.tga"};
    SceneImage DefaultSceneImageBlack{ true, false, 1, "asset/texture/default/black.tga"};
    SceneImage DefaultSceneImageGrey { true, false, 1, "asset/texture/default/grey.tga"};
    SceneImage DefaultSceneImageRed  { true, false, 1, "asset/texture/default/red.tga"};
    SceneImage DefaultSceneImageGreen{ true, false, 1, "asset/texture/default/green.tga"};
    SceneImage DefaultSceneImageBlue { true, false, 1, "asset/texture/default/blue.tga"};
    SceneImage DefaultSceneImageBump { false, false, 1, "asset/texture/default/bump.tga" };
    SceneImage DefaultFogNoiseImage  { false, false, 1, "asset/texture/DensityVolumeTextures/3d_fractal_noise_sample01.tga" };

    MaterialImage DefaultMaterialImageWhite{ DefaultSceneImageWhite, glm::float2(1,1), glm::float2(0,0) };
    MaterialImage DefaultMaterialImageBlack{ DefaultSceneImageBlack, glm::float2(1,1), glm::float2(0,0) };
    MaterialImage DefaultMaterialImageGrey{ DefaultSceneImageGrey, glm::float2(1,1), glm::float2(0,0) };
    MaterialImage DefaultMaterialImageRed{ DefaultSceneImageRed, glm::float2(1,1), glm::float2(0,0) };
    MaterialImage DefaultMaterialImageGreen{ DefaultSceneImageGreen, glm::float2(1,1), glm::float2(0,0) };
    MaterialImage DefaultMaterialImageBlue{ DefaultSceneImageBlue, glm::float2(1,1), glm::float2(0,0) };
    MaterialImage DefaultMaterialImageBump{ DefaultSceneImageBump, glm::float2(1,1), glm::float2(0,0) };
    MaterialImage DefaultMaterialFogNoiseImage{ DefaultFogNoiseImage, glm::float2(1,1), glm::float2(0,0) };

    SceneCommonIdentifier _UndefCommonIdentifier {K_Invalid_Object_Id, K_Invalid_Component_Id};

    MaterialRes ToMaterialRes(const StandardLightMaterial& pbrMaterial, const std::string shaderName)
    {
        MaterialRes m_mat_data{};
        m_mat_data._ShaderName = shaderName;
#define COPY_MATERIAL_PROPERTY(prop) m_mat_data.prop = pbrMaterial.prop
        
        // Texture properties
        COPY_MATERIAL_PROPERTY(_BaseColorMap);
        COPY_MATERIAL_PROPERTY(_MaskMap);
        COPY_MATERIAL_PROPERTY(_NormalMap);
        COPY_MATERIAL_PROPERTY(_NormalMapOS);
        COPY_MATERIAL_PROPERTY(_BentNormalMap);
        COPY_MATERIAL_PROPERTY(_BentNormalMapOS);
        COPY_MATERIAL_PROPERTY(_HeightMap);
        COPY_MATERIAL_PROPERTY(_DetailMap);
        COPY_MATERIAL_PROPERTY(_TangentMap);
        COPY_MATERIAL_PROPERTY(_TangentMapOS);
        COPY_MATERIAL_PROPERTY(_AnisotropyMap);
        COPY_MATERIAL_PROPERTY(_SubsurfaceMaskMap);
        COPY_MATERIAL_PROPERTY(_TransmissionMaskMap);
        COPY_MATERIAL_PROPERTY(_ThicknessMap);
        COPY_MATERIAL_PROPERTY(_IridescenceThicknessMap);
        COPY_MATERIAL_PROPERTY(_IridescenceMaskMap);
        COPY_MATERIAL_PROPERTY(_CoatMaskMap);
        COPY_MATERIAL_PROPERTY(_EmissiveColorMap);
        COPY_MATERIAL_PROPERTY(_TransmittanceColorMap);
        
        // Color and scalar properties
        COPY_MATERIAL_PROPERTY(_BaseColor);
        COPY_MATERIAL_PROPERTY(_Metallic);
        COPY_MATERIAL_PROPERTY(_Smoothness);
        COPY_MATERIAL_PROPERTY(_MetallicRemapMin);
        COPY_MATERIAL_PROPERTY(_MetallicRemapMax);
        COPY_MATERIAL_PROPERTY(_SmoothnessRemapMin);
        COPY_MATERIAL_PROPERTY(_SmoothnessRemapMax);
        COPY_MATERIAL_PROPERTY(_AlphaRemapMin);
        COPY_MATERIAL_PROPERTY(_AlphaRemapMax);
        COPY_MATERIAL_PROPERTY(_AORemapMin);
        COPY_MATERIAL_PROPERTY(_AORemapMax);
        COPY_MATERIAL_PROPERTY(_NormalScale);
        COPY_MATERIAL_PROPERTY(_HeightAmplitude);
        COPY_MATERIAL_PROPERTY(_HeightCenter);
        COPY_MATERIAL_PROPERTY(_HeightMapParametrization);
        COPY_MATERIAL_PROPERTY(_HeightOffset);
        COPY_MATERIAL_PROPERTY(_HeightMin);
        COPY_MATERIAL_PROPERTY(_HeightMax);
        COPY_MATERIAL_PROPERTY(_HeightTessAmplitude);
        COPY_MATERIAL_PROPERTY(_HeightTessCenter);
        COPY_MATERIAL_PROPERTY(_HeightPoMAmplitude);
        COPY_MATERIAL_PROPERTY(_DetailAlbedoScale);
        COPY_MATERIAL_PROPERTY(_DetailNormalScale);
        COPY_MATERIAL_PROPERTY(_DetailSmoothnessScale);
        COPY_MATERIAL_PROPERTY(_Anisotropy);
        COPY_MATERIAL_PROPERTY(_DiffusionProfileHash);
        COPY_MATERIAL_PROPERTY(_SubsurfaceMask);
        COPY_MATERIAL_PROPERTY(_TransmissionMask);
        COPY_MATERIAL_PROPERTY(_Thickness);
        COPY_MATERIAL_PROPERTY(_ThicknessRemap);
        COPY_MATERIAL_PROPERTY(_IridescenceThicknessRemap);
        COPY_MATERIAL_PROPERTY(_IridescenceThickness);
        COPY_MATERIAL_PROPERTY(_IridescenceMask);
        COPY_MATERIAL_PROPERTY(_CoatMask);
        COPY_MATERIAL_PROPERTY(_EnergyConservingSpecularColor);
        COPY_MATERIAL_PROPERTY(_SpecularOcclusionMode);
        COPY_MATERIAL_PROPERTY(_EmissiveColor);
        COPY_MATERIAL_PROPERTY(_AlbedoAffectEmissive);
        COPY_MATERIAL_PROPERTY(_EmissiveIntensityUnit);
        COPY_MATERIAL_PROPERTY(_UseEmissiveIntensity);
        COPY_MATERIAL_PROPERTY(_EmissiveIntensity);
        COPY_MATERIAL_PROPERTY(_EmissiveExposureWeight);
        COPY_MATERIAL_PROPERTY(_UseShadowThreshold);
        COPY_MATERIAL_PROPERTY(_AlphaCutoffEnable);
        COPY_MATERIAL_PROPERTY(_AlphaCutoff);
        COPY_MATERIAL_PROPERTY(_AlphaCutoffShadow);
        COPY_MATERIAL_PROPERTY(_AlphaCutoffPrepass);
        COPY_MATERIAL_PROPERTY(_AlphaCutoffPostpass);
        COPY_MATERIAL_PROPERTY(_TransparentDepthPrepassEnable);
        COPY_MATERIAL_PROPERTY(_TransparentBackfaceEnable);
        COPY_MATERIAL_PROPERTY(_TransparentDepthPostpassEnable);
        COPY_MATERIAL_PROPERTY(_TransparentSortPriority);
        COPY_MATERIAL_PROPERTY(_RefractionModel);
        COPY_MATERIAL_PROPERTY(_Ior);
        COPY_MATERIAL_PROPERTY(_TransmittanceColor);
        COPY_MATERIAL_PROPERTY(_ATDistance);
        COPY_MATERIAL_PROPERTY(_TransparentWritingMotionVec);
        COPY_MATERIAL_PROPERTY(_SurfaceType);
        COPY_MATERIAL_PROPERTY(_BlendMode);
        COPY_MATERIAL_PROPERTY(_SrcBlend);
        COPY_MATERIAL_PROPERTY(_DstBlend);
        COPY_MATERIAL_PROPERTY(_AlphaSrcBlend);
        COPY_MATERIAL_PROPERTY(_AlphaDstBlend);
        COPY_MATERIAL_PROPERTY(_EnableFogOnTransparent);
        COPY_MATERIAL_PROPERTY(_DoubleSidedEnable);
        COPY_MATERIAL_PROPERTY(_DoubleSidedNormalMode);
        COPY_MATERIAL_PROPERTY(_DoubleSidedConstants);
        COPY_MATERIAL_PROPERTY(_DoubleSidedGIMode);
        COPY_MATERIAL_PROPERTY(_UVBase);
        COPY_MATERIAL_PROPERTY(_ObjectSpaceUVMapping);
        COPY_MATERIAL_PROPERTY(_TexWorldScale);
        COPY_MATERIAL_PROPERTY(_UVMappingMask);
        COPY_MATERIAL_PROPERTY(_NormalMapSpace);
        COPY_MATERIAL_PROPERTY(_MaterialID);
        COPY_MATERIAL_PROPERTY(_TransmissionEnable);
        COPY_MATERIAL_PROPERTY(_PPDMinSamples);
        COPY_MATERIAL_PROPERTY(_PPDMaxSamples);
        COPY_MATERIAL_PROPERTY(_PPDLodThreshold);
        COPY_MATERIAL_PROPERTY(_PPDPrimitiveLength);
        COPY_MATERIAL_PROPERTY(_PPDPrimitiveWidth);
        COPY_MATERIAL_PROPERTY(_InvPrimScale);
        COPY_MATERIAL_PROPERTY(_UVDetailsMappingMask);
        COPY_MATERIAL_PROPERTY(_UVDetail);
        COPY_MATERIAL_PROPERTY(_LinkDetailsWithBase);
        COPY_MATERIAL_PROPERTY(_EmissiveColorMode);
        COPY_MATERIAL_PROPERTY(_UVEmissive);
        
#undef COPY_MATERIAL_PROPERTY
        return m_mat_data;
    }

    StandardLightMaterial ToStandardMaterial(const MaterialRes& materialRes)
    {
        StandardLightMaterial m_mat_data{};
#define COPY_MATERIAL_PROPERTY(prop) m_mat_data.prop = materialRes.prop
        
        // Texture properties
        COPY_MATERIAL_PROPERTY(_BaseColorMap);
        COPY_MATERIAL_PROPERTY(_MaskMap);
        COPY_MATERIAL_PROPERTY(_NormalMap);
        COPY_MATERIAL_PROPERTY(_NormalMapOS);
        COPY_MATERIAL_PROPERTY(_BentNormalMap);
        COPY_MATERIAL_PROPERTY(_BentNormalMapOS);
        COPY_MATERIAL_PROPERTY(_HeightMap);
        COPY_MATERIAL_PROPERTY(_DetailMap);
        COPY_MATERIAL_PROPERTY(_TangentMap);
        COPY_MATERIAL_PROPERTY(_TangentMapOS);
        COPY_MATERIAL_PROPERTY(_AnisotropyMap);
        COPY_MATERIAL_PROPERTY(_SubsurfaceMaskMap);
        COPY_MATERIAL_PROPERTY(_TransmissionMaskMap);
        COPY_MATERIAL_PROPERTY(_ThicknessMap);
        COPY_MATERIAL_PROPERTY(_IridescenceThicknessMap);
        COPY_MATERIAL_PROPERTY(_IridescenceMaskMap);
        COPY_MATERIAL_PROPERTY(_CoatMaskMap);
        COPY_MATERIAL_PROPERTY(_EmissiveColorMap);
        COPY_MATERIAL_PROPERTY(_TransmittanceColorMap);
        
        // Color and scalar properties
        COPY_MATERIAL_PROPERTY(_BaseColor);
        COPY_MATERIAL_PROPERTY(_Metallic);
        COPY_MATERIAL_PROPERTY(_Smoothness);
        COPY_MATERIAL_PROPERTY(_MetallicRemapMin);
        COPY_MATERIAL_PROPERTY(_MetallicRemapMax);
        COPY_MATERIAL_PROPERTY(_SmoothnessRemapMin);
        COPY_MATERIAL_PROPERTY(_SmoothnessRemapMax);
        COPY_MATERIAL_PROPERTY(_AlphaRemapMin);
        COPY_MATERIAL_PROPERTY(_AlphaRemapMax);
        COPY_MATERIAL_PROPERTY(_AORemapMin);
        COPY_MATERIAL_PROPERTY(_AORemapMax);
        COPY_MATERIAL_PROPERTY(_NormalScale);
        COPY_MATERIAL_PROPERTY(_HeightAmplitude);
        COPY_MATERIAL_PROPERTY(_HeightCenter);
        COPY_MATERIAL_PROPERTY(_HeightMapParametrization);
        COPY_MATERIAL_PROPERTY(_HeightOffset);
        COPY_MATERIAL_PROPERTY(_HeightMin);
        COPY_MATERIAL_PROPERTY(_HeightMax);
        COPY_MATERIAL_PROPERTY(_HeightTessAmplitude);
        COPY_MATERIAL_PROPERTY(_HeightTessCenter);
        COPY_MATERIAL_PROPERTY(_HeightPoMAmplitude);
        COPY_MATERIAL_PROPERTY(_DetailAlbedoScale);
        COPY_MATERIAL_PROPERTY(_DetailNormalScale);
        COPY_MATERIAL_PROPERTY(_DetailSmoothnessScale);
        COPY_MATERIAL_PROPERTY(_Anisotropy);
        COPY_MATERIAL_PROPERTY(_DiffusionProfileHash);
        COPY_MATERIAL_PROPERTY(_SubsurfaceMask);
        COPY_MATERIAL_PROPERTY(_TransmissionMask);
        COPY_MATERIAL_PROPERTY(_Thickness);
        COPY_MATERIAL_PROPERTY(_ThicknessRemap);
        COPY_MATERIAL_PROPERTY(_IridescenceThicknessRemap);
        COPY_MATERIAL_PROPERTY(_IridescenceThickness);
        COPY_MATERIAL_PROPERTY(_IridescenceMask);
        COPY_MATERIAL_PROPERTY(_CoatMask);
        COPY_MATERIAL_PROPERTY(_EnergyConservingSpecularColor);
        COPY_MATERIAL_PROPERTY(_SpecularOcclusionMode);
        COPY_MATERIAL_PROPERTY(_EmissiveColor);
        COPY_MATERIAL_PROPERTY(_AlbedoAffectEmissive);
        COPY_MATERIAL_PROPERTY(_EmissiveIntensityUnit);
        COPY_MATERIAL_PROPERTY(_UseEmissiveIntensity);
        COPY_MATERIAL_PROPERTY(_EmissiveIntensity);
        COPY_MATERIAL_PROPERTY(_EmissiveExposureWeight);
        COPY_MATERIAL_PROPERTY(_UseShadowThreshold);
        COPY_MATERIAL_PROPERTY(_AlphaCutoffEnable);
        COPY_MATERIAL_PROPERTY(_AlphaCutoff);
        COPY_MATERIAL_PROPERTY(_AlphaCutoffShadow);
        COPY_MATERIAL_PROPERTY(_AlphaCutoffPrepass);
        COPY_MATERIAL_PROPERTY(_AlphaCutoffPostpass);
        COPY_MATERIAL_PROPERTY(_TransparentDepthPrepassEnable);
        COPY_MATERIAL_PROPERTY(_TransparentBackfaceEnable);
        COPY_MATERIAL_PROPERTY(_TransparentDepthPostpassEnable);
        COPY_MATERIAL_PROPERTY(_TransparentSortPriority);
        COPY_MATERIAL_PROPERTY(_RefractionModel);
        COPY_MATERIAL_PROPERTY(_Ior);
        COPY_MATERIAL_PROPERTY(_TransmittanceColor);
        COPY_MATERIAL_PROPERTY(_ATDistance);
        COPY_MATERIAL_PROPERTY(_TransparentWritingMotionVec);
        COPY_MATERIAL_PROPERTY(_SurfaceType);
        COPY_MATERIAL_PROPERTY(_BlendMode);
        COPY_MATERIAL_PROPERTY(_SrcBlend);
        COPY_MATERIAL_PROPERTY(_DstBlend);
        COPY_MATERIAL_PROPERTY(_AlphaSrcBlend);
        COPY_MATERIAL_PROPERTY(_AlphaDstBlend);
        COPY_MATERIAL_PROPERTY(_EnableFogOnTransparent);
        COPY_MATERIAL_PROPERTY(_DoubleSidedEnable);
        COPY_MATERIAL_PROPERTY(_DoubleSidedNormalMode);
        COPY_MATERIAL_PROPERTY(_DoubleSidedConstants);
        COPY_MATERIAL_PROPERTY(_DoubleSidedGIMode);
        COPY_MATERIAL_PROPERTY(_UVBase);
        COPY_MATERIAL_PROPERTY(_ObjectSpaceUVMapping);
        COPY_MATERIAL_PROPERTY(_TexWorldScale);
        COPY_MATERIAL_PROPERTY(_UVMappingMask);
        COPY_MATERIAL_PROPERTY(_NormalMapSpace);
        COPY_MATERIAL_PROPERTY(_MaterialID);
        COPY_MATERIAL_PROPERTY(_TransmissionEnable);
        COPY_MATERIAL_PROPERTY(_PPDMinSamples);
        COPY_MATERIAL_PROPERTY(_PPDMaxSamples);
        COPY_MATERIAL_PROPERTY(_PPDLodThreshold);
        COPY_MATERIAL_PROPERTY(_PPDPrimitiveLength);
        COPY_MATERIAL_PROPERTY(_PPDPrimitiveWidth);
        COPY_MATERIAL_PROPERTY(_InvPrimScale);
        COPY_MATERIAL_PROPERTY(_UVDetailsMappingMask);
        COPY_MATERIAL_PROPERTY(_UVDetail);
        COPY_MATERIAL_PROPERTY(_LinkDetailsWithBase);
        COPY_MATERIAL_PROPERTY(_EmissiveColorMode);
        COPY_MATERIAL_PROPERTY(_UVEmissive);
        
#undef COPY_MATERIAL_PROPERTY
        return m_mat_data;
    }

    // clang-format off

    //--------------------------------------------------------------------------------------
    // Vertex struct holding position information.
    const D3D12_INPUT_ELEMENT_DESC D3D12MeshVertexPosition::InputElements[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static_assert(sizeof(D3D12MeshVertexPosition) == 12, "Vertex struct/layout mismatch");

    const RHI::D3D12InputLayout D3D12MeshVertexPosition::InputLayout = 
        RHI::D3D12InputLayout( D3D12MeshVertexPosition::InputElements, 
            D3D12MeshVertexPosition::InputElementCount);

    //--------------------------------------------------------------------------------------
    // Vertex struct holding position and texture mapping information.
    const D3D12_INPUT_ELEMENT_DESC D3D12MeshVertexPositionTexture::InputElements[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static_assert(sizeof(D3D12MeshVertexPositionTexture) == 20, "Vertex struct/layout mismatch");

    const RHI::D3D12InputLayout D3D12MeshVertexPositionTexture::InputLayout = 
        RHI::D3D12InputLayout( D3D12MeshVertexPositionTexture::InputElements, 
            D3D12MeshVertexPositionTexture::InputElementCount);

    //--------------------------------------------------------------------------------------
    // Vertex struct holding position, normal, tangent and texture mapping information.
    const D3D12_INPUT_ELEMENT_DESC D3D12MeshVertexPositionNormalTangentTexture::InputElements[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static_assert(sizeof(D3D12MeshVertexPositionNormalTangentTexture) == 48, "Vertex struct/layout mismatch");

    const RHI::D3D12InputLayout D3D12MeshVertexPositionNormalTangentTexture::InputLayout = 
        RHI::D3D12InputLayout( D3D12MeshVertexPositionNormalTangentTexture::InputElements, 
            D3D12MeshVertexPositionNormalTangentTexture::InputElementCount);

    //--------------------------------------------------------------------------------------
    // Vertex struct holding position, normal, tangent and texture mapping information.
    const D3D12_INPUT_ELEMENT_DESC D3D12MeshVertexPositionNormalTangentTextureJointBinding::InputElements[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES",0, DXGI_FORMAT_R32G32B32A32_UINT,  0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static_assert(sizeof(D3D12MeshVertexPositionNormalTangentTextureJointBinding) == 80, "Vertex struct/layout mismatch");

    const RHI::D3D12InputLayout D3D12MeshVertexPositionNormalTangentTextureJointBinding::InputLayout = 
        RHI::D3D12InputLayout( D3D12MeshVertexPositionNormalTangentTextureJointBinding::InputElements, 
            D3D12MeshVertexPositionNormalTangentTextureJointBinding::InputElementCount);

    //--------------------------------------------------------------------------------------
    const D3D12_INPUT_ELEMENT_DESC D3D12MeshVertexStandard::InputElements[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static_assert(sizeof(D3D12MeshVertexStandard) == 48, "Vertex struct/layout mismatch");

    const RHI::D3D12InputLayout D3D12MeshVertexStandard::InputLayout = 
        RHI::D3D12InputLayout(D3D12MeshVertexStandard::InputElements, D3D12MeshVertexStandard::InputElementCount);

    //--------------------------------------------------------------------------------------
    // Vertex struct holding position, normal, tangent and texture mapping information.
    const D3D12_INPUT_ELEMENT_DESC D3D12TerrainPatch::InputElements[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",       0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static_assert(sizeof(D3D12TerrainPatch) == 64, "Vertex struct/layout mismatch");

    const RHI::D3D12InputLayout D3D12TerrainPatch::InputLayout = 
        RHI::D3D12InputLayout( D3D12TerrainPatch::InputElements, D3D12TerrainPatch::InputElementCount);


    // clang-format on



}